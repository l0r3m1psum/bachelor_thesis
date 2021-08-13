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

create table maps (
	id   int4            generated always as identity primary key,
	name str             unique,
	rect "map rectangle",
	data parameters[][]  not null check (array_ndims(data) = 2)
	constraint "right dimension" check (
		((rect).x2 - (rect).x1 + 1) * ((rect).y2 - (rect).y1 + 1)
		= array_length(data,1) * array_length(data, 2)
	)
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

-- TODO: add seq ordering trigger and data's area must be equal to its simulation rectangle area.
create table results (
	sim int4 references simualtions(id),
	seq int2,
	primary key (sim, seq),
	data state[][]   not null check (array_ndims(data) = 2)
);

create function "rectangle inside function"() returns trigger as $$
declare
	"map rect" "map rectangle" := row(0, 0, 1, 1);
begin
	"map rect" := (select rect from maps where id = new.map);

	if      (new.rect).x1 >= ("map rect").x1
		and (new.rect).x2 <= ("map rect").x2
		and (new.rect).y1 >= ("map rect").y1
		and (new.rect).y2 <= ("map rect").y2 then
		return new;
	else
		raise exception 'a simulation rectangle must be inside its map'
		'rectangle, and % is not inside %', new.rect, "map rect";
	end if;
end;
$$ language plpgsql;

create trigger "rectangle inside trigger" before insert or update
	on simualtions
	for each row
	execute function "rectangle inside function"();

create function "same area function"() returns trigger as $$
declare
	"sim rect" "map rectangle" := row(0, 0, 1, 1);
	"area res" int4 := 0;
	"area sim" int4 := 0;
begin
	"sim rect" := (select rect from simualtions where id = new.sim);
	"area sim" := (("sim rect").x2 - ("sim rect").x1 + 1) * (("sim rect").y2 - ("sim rect").y1 + 1);
	"area res" := array_length(new.data,1) * array_length(new.data, 2);

	if "area sim" = "area res" then
		return new;
	else
		raise exception 'a result data must have the same area of its'
		' simulation, and % is not equal to %', "area sim", "area res";
	end if;
end;
$$ language plpgsql;

create trigger "same area trigger" before insert or update
	on results
	for each row
	execute function "same area function"();

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

commit;
