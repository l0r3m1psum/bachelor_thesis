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
create table map (
	x                int4   check (x >= 0),
	y                int4   check (x >= 0),
	primary key (x, y),
	altimetry        int2   not null check (altimetry between 0 and 4380),
	forest           int2   not null check (forest between 0 and 2 or forest = 255),
	urbanization     int2   not null check (urbanization between 0 and 255),
	water1           int2   not null check (water1 between 0 and 4 or water1 = 253 or water1 = 255),
	water2           bool   not null,
	"carta natura"   int2   not null check ("carta natura" between 1 and 90),
	"wind direction" float8 not null check ("wind direction" between -pi() and pi()),
	"wind speed"     float8 not null check ("wind speed" >= 0)
);

create table results (
	ts    timestamp, -- 8 bytes
	x     int4  check (x >= 0),
	y     int4  check (x >= 0),
	primary key (ts, x, y),
	fuel  float8 not null check (fuel >= 0),
	state bool   not null,
);

commit;
