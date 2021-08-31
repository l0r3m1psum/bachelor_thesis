\set ON_ERROR_STOP on
-- \set QUIET on
\pset footer off
\pset border 2
\pset linestyle unicode
\set db test
drop database if exists :db;
create database :db;
\c :db
-- set client_min_messages = debug;

-- DOC: https://www.postgresql.org/docs/13/rowtypes.html

-- TODO: should the rectangles be or in our piece of earth?
begin;

-- TYPES and DOMAINS -----------------------------------------------------------

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

-- FUNCTIONS -------------------------------------------------------------------

-- TODO: use an appropiate return type to avoid overflow exception
create function matrixArea(matrix anyelement) returns int4 as $$
-- TODO: add pg_typeof to make it "type safe"
select array_length(matrix, 1) * array_length(matrix, 2)
$$ language sql;

-- TODO: use an appropiate return type to avoid overflow exception
create function rectArea(rect "map rectangle") returns int4 as $$
select ((rect).x2 - (rect).x1 + 1) * ((rect).y2 - (rect).y1 + 1)
$$ language sql;

create function rectInside(r1 "map rectangle", r2 "map rectangle") returns bool as $$
select (r1).x1 >= (r2).x1
	and (r1).x2 <= (r2).x2
	and (r1).y1 >= (r2).y1
	and (r1).y2 <= (r2).y2
$$ language sql;

comment on function rectInside is 'Checks is r1 is inside r2.';

-- TABLES ----------------------------------------------------------------------

create table maps (
	id   int4            generated always as identity primary key,
	name str             unique,
	unit int2 not null check (unit > 0),
	rect "map rectangle",
	data parameters[][]  not null check (array_ndims(data) = 2)
	constraint "right area" check (rectArea(rect) = matrixArea(data))
);

create table simualtions (
	id int4 generated always as identity primary key,
	name str unique,
	rect "map rectangle",
	map int4 not null references maps(id),
	horizon int4 check(horizon > 0),
	-- etc.
	started timestamp not null default current_timestamp
);

create table results (
	sim int4 references simualtions(id),
	seq int2,
	primary key (sim, seq),
	data state[][]   not null check (array_ndims(data) = 2)
);

-- TRIGGERS --------------------------------------------------------------------

\include triggers.sql

-- TESTING ---------------------------------------------------------------------

insert into maps(name, unit, rect, data) values (
	'test map',
	1,
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

update maps set rect = row(0, 0, 3, 3) where id = 1;
update maps set data[1][1] = row(2, 1, 1, 1, true, 1, 1, 1) where id = 1;
update results set data[1][1] = row(0, true) where sim = 1;

delete from results;
delete from simualtions;
delete from maps;

select * from maps;
select * from simualtions;
select * from results;

commit;

vacuum (full, analyze);
