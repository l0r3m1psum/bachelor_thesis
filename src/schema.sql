\set ON_ERROR_STOP on
-- \set QUIET on
\pset footer off
\pset border 2
\pset linestyle unicode
\set db test
drop database if exists :db;
create database :db;
\c :db

-- DOC: https://www.postgresql.org/docs/13/rowtypes.html

-- TODO: should the rectangles be or in our piece of earth?
begin;

create type "map rectangle implementation" as (x1 int4, y1 int4, x2 int4, y2 int4);
create domain "map rectangle" as "map rectangle implementation" not null check (
	    ((value).x1 is not null and (value).x1 >= 0)
	and ((value).y1 is not null and (value).y1 >= 0)
	and ((value).x2 is not null and (value).x2 >= 0)
	and ((value).y2 is not null and (value).y2 >= 0)
	and ((value).x1 < (value).x2)
	and ((value).y1 < (value).y2)
);

create type "parameters implementation" as (
	altimetry  int2,
	forest int2,
	urbanization int2,
	water1 int2,
	water2 bool, -- NOTE: currently unused by the model
	"carta natura" int2, -- NOTE: currently unused by the model
	"wind angle"  float8,
	"wind speed"  float8
);
create domain parameters as "parameters implementation" not null check (
	    ((value).altimetry      is not null and (value).altimetry between 0 and 4380)
	and ((value).forest         is not null and (value).forest between 0 and 2 or (value).forest = 255)
	and ((value).urbanization   is not null and (value).urbanization between 0 and 255)
	and ((value).water1         is not null and (value).water1 between 0 and 4 or (value).water1 = 253 or (value).water1 = 255)
	and ((value).water2         is not null)
	and ((value)."carta natura" is not null and (value)."carta natura" between 1 and 90)
	and ((value)."wind angle"   is not null and (value)."wind angle" between -pi() and pi())
	and ((value)."wind speed"   is not null and (value)."wind speed" >= 0)
);

create type "state implementation" as (fuel float8, "on fire" bool);
create domain state as "state implementation" not null check (
	    ((value).fuel      is not null and (value).fuel >= 0)
	and ((value)."on fire" is not null)
);

create domain str as text   not null; -- TODO add check to avoid absurd characters like emoji and the non printable ones

--------------------------------------------------------------------------------

-- TODO: use an appropiate return type to avoid overflow exception
create function matrixArea(matrix anyelement) returns int4 as $$
-- TODO: add pg_typeof to make it "type safe"
select array_length(matrix, 1) * array_length(matrix, 2)
$$ language sql;

-- TODO: use an appropiate return type to avoid overflow exception
create function rectArea(rect "map rectangle") returns int4 as $$
select ((rect).x2 - (rect).x1 + 1) * ((rect).y2 - (rect).y1 + 1)
$$ language sql;

create function rectInsideAnother(r1 "map rectangle", r2 "map rectangle") returns bool as $$
select (r1).x1 >= (r2).x1
	and (r1).x2 <= (r2).x2
	and (r1).y1 >= (r2).y1
	and (r1).y2 <= (r2).y2
$$ language sql;

comment on function rectInsideAnother is 'Checks is r1 is inside r2.';

--------------------------------------------------------------------------------

create table maps (
	id   int4            generated always as identity primary key,
	name str             unique,
	rect "map rectangle",
	data parameters[][]  not null check (array_ndims(data) = 2)
	constraint "right dimension" check (rectArea(rect) = matrixArea(data))
);

create table simualtions (
	id int4 generated always as identity primary key,
	name str unique,
	rect "map rectangle",
	map int4 not null references maps(id),
	horizon float8,
	-- etc.
	started timestamp not null default current_timestamp
);

create table results (
	sim int4 references simualtions(id),
	seq int2,
	primary key (sim, seq),
	data state[][]   not null check (array_ndims(data) = 2)
);

--------------------------------------------------------------------------------

create function "rectangle inside function"() returns trigger as $$
declare
	"map rect" "map rectangle" := row(0, 0, 1, 1);
	x1 int4;
	y1 int4;
	x2 int4;
	y2 int4;
begin
	assert TG_WHEN = 'BEFORE' and TG_LEVEL = 'ROW';

	if TG_RELNAME = 'maps' then
		if TG_OP = 'INSERT' then
			return new;
		elsif TG_OP = 'UPDATE' then
			for x1, y1, x2, y2 in select rect from simualtions where map = new.id loop
				if not rectInsideAnother(row(x1, y1, x2, y2), new.rect) then
					raise exception 'all simulation on this map must be inside it';
				end if;
			end loop;
			return new;
		elsif TG_OP = 'DELETE' then
			return old;
		else
			raise exception 'not implemented for operation %', TG_OP;
		end if;
	elsif TG_RELNAME = 'simualtions' then
		if TG_OP = 'INSERT' then
			"map rect" := (select rect from maps where id = new.map);

			if rectInsideAnother(new.rect, "map rect") then
				return new;
			else
				raise exception 'a simulation rectangle must be inside its map'
				'rectangle, and % is not inside %', new.rect, "map rect";
			end if;
		elsif TG_OP = 'UPDATE' then
			"map rect" := (select rect from maps where id = new.map);

			if rectInsideAnother(new.rect, "map rect") then
				return new;
			else
				raise exception 'a simulation rectangle must be inside its map'
				'rectangle, and % is not inside %', new.rect, "map rect";
			end if;
		elsif TG_OP = 'DELETE' then
			return old;
		else
			raise exception 'not implemented for operation %', TG_OP;
		end if;
	else
		raise exception 'not implemented for table %', TG_RELNAME;
	end if;
end;
$$ language plpgsql;

create trigger "rectangle inside" before insert or update or delete
	on maps
	for each row
	execute function "rectangle inside function"();

create trigger "rectangle inside" before insert or update or delete
	on simualtions
	for each row
	execute function "rectangle inside function"();

create function "same area function"() returns trigger as $$
declare
	"sim rect" "map rectangle" := row(0, 0, 1, 1);
	"area res" int4;
	"area sim" int4;
begin
	assert TG_WHEN = 'BEFORE' and TG_LEVEL = 'ROW';

	if TG_RELNAME = 'results' then
		if TG_OP = 'INSERT' then
			"sim rect" := (select rect from simualtions where id = new.sim);
			"area sim" := rectArea("sim rect");
			"area res" := matrixArea(new.data);

			if "area sim" = "area res" then
				return new;
			else
				raise exception 'a result data must have the same area of its'
				' simulation, and % is not equal to %', "area sim", "area res";
			end if;
		elsif TG_OP = 'UPDATE' then
			if matrixArea(new.rect) <> matrixArea(old.rect) then
				raise exception 'the area must remain equal';
			else
				return new;
			end if;
		elsif TG_OP = 'DELETE' then
			return old;
		else
			raise exception 'not implemented for operation %', TG_OP;
		end if;
	elsif TG_RELNAME = 'simualtions' then
		if TG_OP = 'INSERT' then
			return new;
		elsif TG_OP = 'UPDATE' then
			-- TODO: look into this
			if new."map rect" <> old."map rect" then
				raise exception 'the "map rect" cannot be modified';
			else
				return new;
			end if;
		elsif TG_OP = 'DELETE' then
			return old;
		else
			raise exception 'not implemented for operation %', TG_OP;
		end if;
	elsif TG_RELNAME = 'maps' then
		if TG_OP = 'INSERT' then
			return new;
		elsif TG_OP = 'UPDATE' then
			if matrixArea(new.data) <> matrixArea(old.data) then
				raise exception 'the area must remain equal';
			else
				return new;
			end if;
		elsif TG_OP = 'DELETE' then
			return old;
		else
			raise exception 'not implemented for operation %', TG_OP;
		end if;
	else
		raise exception 'not implemented for table %', TG_RELNAME;
	end if;
end;
$$ language plpgsql;

create trigger "same area" before insert or update or delete
	on results
	for each row
	execute function "same area function"();

create trigger "same area" before insert or update or delete
	on simualtions
	for each row
	execute function "same area function"();

create trigger "same area" before insert or update or delete
	on maps
	for each row
	execute function "same area function"();

create function "number sequence function"() returns trigger as $$
declare
	"current max" int4;
begin
	assert TG_WHEN = 'BEFORE' and TG_LEVEL = 'ROW' and TG_RELNAME = 'results';
	if TG_OP = 'INSERT' then
		"current max" := (select coalesce(max(seq), -1) from results where sim = new.sim);

		if new.seq = "current max" + 1 then
			return new;
		else
			raise exception 'the sequence of results must increase by one on every'
			'entry, and % is not the soccessor of %', new.seq, "current max";
		end if;
	elsif TG_OP = 'UPDATE' then
		if new.seq <> old.seq then
			raise exception 'the sequence number cannot be modified';
		else
			return new;
		end if;
	elsif TG_OP = 'DELETE' then
		"current max" := (select max(seq) from results where sim = old.sim);

		if old.seq = "current max" then
			return old;
		else
			raise exception 'only the last result in the sequence can be deleted';
		end if;
	else
		raise exception 'not implemented for operation %', TG_OP;
	end if;
end;
$$ language plpgsql;

comment on function "number sequence function" is 'Mantains the invariant that'
' the seq column of each simulation must be a successor (+1) of its predecessor'
'(in the Peano natural numbers sense).';

create trigger "number sequence" before insert or update or delete
	on results
	for each row
	execute function "number sequence function"();

--------------------------------------------------------------------------------

insert into maps(name, rect, data) values (
	'test map',
	row(0, 0, 2, 2),
	cast(array[
		array[row(1, 1, 1, 1, true, 1, 1, 1), row(1, 1, 1, 1, true, 1, 1, 1), row(1, 1, 1, 1, true, 1, 1, 1)],
		array[row(1, 1, 1, 1, true, 1, 1, 1), row(1, 1, 1, 1, true, 1, 1, 1), row(1, 1, 1, 1, true, 1, 1, 1)],
		array[row(1, 1, 1, 1, true, 1, 1, 1), row(1, 1, 1, 1, true, 1, 1, 1), row(1, 1, 1, 1, true, 1, 1, 1)]
	] as parameters[][])
);

insert into simualtions(name, rect, map, horizon) values
	('test simualtion1', row(0, 0, 2, 2), 1, 10),
	('test simualtion2', row(0, 0, 2, 2), 1, 10),
	('test simualtion3', row(0, 0, 2, 2), 1, 10);

insert into results(sim, seq, data) values
	(1, 0, cast(array[
		array[row(0, false), row(0, false), row(0, false)],
		array[row(0, false), row(0, false), row(0, false)],
		array[row(0, false), row(0, false), row(0, false)]
	] as state[][])),
	(1, 1, cast(array[
		array[row(0, false), row(0, false), row(0, false)],
		array[row(0, false), row(0, false), row(0, false)],
		array[row(0, false), row(0, false), row(0, false)]
	] as state[][]));

delete from results where sim = 1 and seq = 1;

-- update maps set rect = row(0, 0, 3, 3) where id = 1;

commit;
