\set ON_ERROR_STOP on
\set QUIET on
\pset footer off
\pset border 2
\pset linestyle unicode
drop database if exists :db;
create database :db;
\c :db

begin;
create table map (
	-- TODO: find a base for x and y to check 10 meter distance
	x              integer  not null check (x >= 0 and (x - 4559505)%10 = 0),
	y              integer  not null check (y >= 0 and (y - 2130995)%10 = 0),
	altimetry      smallint not null check (altimetry between 0 and 4380),
	forest         smallint not null check (forest between 0 and 2
		or forest = 255),
	urbanization   smallint not null check (urbanization between 0 and 255),
	water1         smallint not null check (water1 between 0 and 4
		or water1 = 253 or water1 = 255),
	water2         boolean  not null,
	"carta natura" smallint not null check ("carta natura" between 1 and 90),
	primary key (x, y)
);

create table meteo (
	x integer not null,
	y integer not null,
	"wind speed" float8 not null,
	"wind direction" float8 not null,
	foreign key (x, y) references map(x, y)
);

commit;
