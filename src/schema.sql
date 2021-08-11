\set ON_ERROR_STOP on
\set QUIET on
\pset footer off
\pset border 2
\pset linestyle unicode
\set db test
drop database if exists :db;
create database :db;
\c :db

-- TODO: should the boxes be or in our piece of earth?
begin;

create domain ufloat8        as float8 not null check (value >= 0);
create domain uint4          as int4   not null check (value >= 0);
create domain bool1          as bool   not null;
create domain angle          as float8 not null check (value between -pi() and pi());
create domain altimetry      as int2   not null check (value between 0 and 4380);
create domain forest         as int2   not null check (value between 0 and 2 or value = 255);
create domain urbanization   as int2   not null check (value between 0 and 255);
create domain water1         as int2   not null check (value between 0 and 4 or value = 253 or value = 255);
create domain "carta natura" as int2   not null check (value between 1 and 90);

create type parameters as (
	p  altimetry,
	g  forest,
	u  urbanization,
	w1 water1,
	w2 bool1, -- NOTE: currently unused by the model
	c  "carta natura", -- NOTE: currently unused by the model
	d  angle, -- "wind direction"
	f  ufloat8 -- "wind speed"
);

create type state as (
	fuel ufloat8,
	"on fire" bool1
);

create type "map point" as (
	x uint4,
	y uint4
);

create table map (
	id   int4           generated always as identity primary key,
	p1   "map point"    not null,
	p2   "map point"    not null,
	data parameters[][] not null check (array_ndims(data) = 2)
);

create table results (
	id   int4        generated always as identity primary key,
	ts   timestamp   unique not null default current_timestamp,
	p1   "map point" not null,
	p2   "map point" not null,
	data state[][]   not null check (array_ndims(data) = 2) -- array_length(anyarray, int)
);

commit;
