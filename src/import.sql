-- Script per importare dati
--
--   * trovare in minimo punto, il massimo punto e in numero di punto
--   * calcolare l'area del rettangolo, vedere se il numero di punti corrisponde
--   * inserire tutti i dati

\c test

create temporary table geography (
	x text not null,
	y text not null,
	p text not null, -- altimetria
	g text not null, -- foreste
	u text not null, -- urbanizzazione
	w1 text not null, -- water 1
	w2 text not null, -- water 2
	c text not null -- carta natura
);

\copy geography from '../res/turano_geografia.csv' with (format csv);

create temporary table meteorology (
	x text not null,
	y text not null,
	s text not null, -- speed
	d text not null -- direction
);

\copy meteorology from '../res/turano_meteo.csv' with (format csv);

with "sane geo" as (
	select
		-- removing the last tre zeros
		cast(substring(sub.x for char_length(sub.x)-3) as int4) as x,
		cast(substring(sub.y for char_length(sub.y)-3) as int4) as y,
		cast(p as int2),
		cast(g as int2),
		cast(u as int2),
		cast(w1 as int2),
		cast(w2 as bool),
		cast(c as int2)
	from (select replace(x, '.', '') as x, replace(y, '.', '') as y, p, g, u, w1, w2, c from geography) as sub
), "sane meteo" as (
	select
		-- removing the last tre zeros
		cast(substring(sub.x for char_length(sub.x)-3) as int4) as x,
		cast(substring(sub.y for char_length(sub.y)-3) as int4) as y,
		cast(s as float8),
		cast(d as float8)
	from (select replace(x, '.', '') as x, replace(y, '.', '')as y, s, d  from meteorology) as sub
), "all parameters" as (
	select x, y, p, g, u, w1, w2, c, s, d
	from "sane geo" natural join "sane meteo"
), "min max count geo" as (
	select
		min(x)/10 as x1,
		min(y)/10 as y1,
		max(x)/10 as x2,
		max(y)/10 as y2,
		count(*) as count
	from "sane geo"
), "min max count meteo" as (
	select
		min(x)/10 as x1,
		min(y)/10 as y1,
		max(x)/10 as x2,
		max(y)/10 as y2,
		count(*) as count
	from "sane meteo"
), dimensions as (
	select *
	from "min max count geo" natural join "min max count meteo"
), "ordered records" as (
	select y, array_agg(cast(row(p, g, u, w1, w2, c, s, d) as parameters)) as params
	from "all parameters"
	group by y
	order by y desc
), res as (
	select
		(select cast(row(x1, y1, x2, y2) as "map rectangle") from dimensions) as rect,
		cast(array(select params from "ordered records") as parameters[][]) as data
) insert into maps(name, rect, data)
	select 'turano', r.rect, r.data from res as r;
-- TODO: assicurarsi che gli elementi in res.data siano ordinati correttamente
-- TODO: le coordinate sono tutte spaziate di 10 metri l'una dall'altra, questo
--       rompe la formula per calcolare l'area. Quindi bisogna salvare la grana
--       dei dati in maps e bisogna scegliere come rappresentare i dati nei rect
--       o come coordinate vere (1:1) o scalate dalla grana, (come ho fatto
--       sopra) il primo metodo richiede l'aggiunta di un parametro alla
--       funzione che calcola l'area dei rettangoli, e potrebbe rendere più
--       facile la modifica erronea dei dati (e.g. spostare un rettangolo di 0.5
--       unità di grana).
-- NOTE: un nome miggliore per la grana sarebbe tipo "unità di misura" o simile
select name, rect, data[1][1] from maps;

vacuum full;
