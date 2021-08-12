\set ON_ERROR_STOP on
\set QUIET on
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
-- Types and domains for the map rectangle
create domain uint4 as int4 not null check (value >= 0);
create type "uint4 pair" as (x uint4, y uint4);
create domain "map point" as "uint4 pair" not null check ((value).x >= 0 and (value).y >= 0);
create type "map point pair" as (p1 "map point", p2 "map point");
create domain "map rectangle" as "map point pair" not null check ((value).p1.x < (value).p2.x and (value).p1.y < (value).p2.y);

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
	((value).fuel is not null and (value).fuel >= 0)
	and ((value)."on fire" is not null)
);

create domain str as text   not null; -- TODO add check to avoid absurd characters like emoji and the non printable ones

create table maps (
	id   int4            generated always as identity primary key,
	name str             unique,
	rect "map rectangle",
	data parameters[][]  not null check (array_ndims(data) = 2)
);

create table simualtions (
	id int4 generated always as identity primary key,
	name str unique,
	p1 "map point" not null,
	p2 "map point" not null,
	map int4 not null references maps(id),
	horizon float8,
	-- etc.
	started timestamp not null default current_timestamp
	constraint "points order" check ((p1).x < (p2).x and (p1).y < (p2).y)
	-- constraint "right dimension" check (((p2).x - (p1).x) * ((p2).y - (p1).y) = array_length(data,1) * array_length(data, 2))
);

create table results (
	simualtion int4 references simualtions(id),
	order__ int2,
	primary key (simualtion, order__),
	data state[][]   not null check (array_ndims(data) = 2)
);

commit;
