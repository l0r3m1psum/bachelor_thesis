-- Devono essere mantenute tre invarianze:
--   1. Contenimento dei rettangoli (tra maps e simulations)
--   2. L'area di results.data deve essere la stessa di simulation.rect
--   3. I numeri di results.seq devono essere una sequanza di numeri che partono
--      da 0 e incrementano di 1, per ogni results.sim
--
-- Queste invarianti posso essere violate con le varie operazioni sulle tabelle:
--   * INSERT
--     - INTO simulations -> rectInside(new.rect, map.rect)
--     - INTO results -> matrixArea(new.data) = rectArea(simulations.rect)
--     - INTO results -> new.seq = max(seq) + 1
--
--   * UPDATE
--     - OF maps.rect -> tutte le simulazioni devono rimanere dentro map.rect
--     - OF simulations.rect -> rectInside(new.rect, maps.rect)
--     - OF results.data -> matrixArea(new.data) = rectArea(simulations.rect)
--
--   * DELETE
--     - FROM results.seq -> deve candellare sempre l'ultimo.

-- INSERTS ---------------------------------------------------------------------

create function "check simulations insert rect"() returns trigger as $$
begin
	return (select case
		when rectInside(new.rect, (select rect from maps where id = new.map)) then new
		else null end);
end;
$$ language plpgsql;

create trigger "simulations insert rect" before insert on simualtions
	for each row execute function "check simulations insert rect"();

create function "check results inserts data"() returns trigger as $$
begin
	return (select case
		when matrixArea(new.data) = rectArea((select rect from simualtions where id = new.sim)) then new
		else null end);
end;
$$ language plpgsql;

create trigger "results insert data" before insert on results
	for each row execute function "check results inserts data"();

create function "check results insert seq"() returns trigger as $$
begin
	return (with "current max" as (
		select coalesce(max(seq), -1) as max from results where sim = new.sim
	) select case
		when new.seq = (select max from "current max") + 1 then new
		else null end);
end;
$$ language plpgsql;

create trigger "results insert seq" before insert on results
	for each row execute function "check results insert seq"();

-- UPDATE ----------------------------------------------------------------------

create function "check maps update rect"() returns trigger as $$
begin
	return (with "contained rects" as (
		select rectInside(new.rect, rect) as isInside
		from simualtions
		where map = new.id
	) select case
		when not exists (select isInside from "contained rects" where not isInside) then new
		else null end);
end;
$$ language plpgsql;

create trigger "maps update rect" before update of rect on maps
	for each row execute function "check maps update rect"();

create trigger "simulations update rect" before update of rect on simualtions
	for each row execute function "check simulations insert rect"();

create trigger "results update data" before update of data on results
	for each row execute function "check results inserts data"();

-- DELETE ----------------------------------------------------------------------

create function "check results delete seq"() returns trigger as $$
begin
	return (select case
		when old.seq = (select max(seq) from results where sim = old.sim) then old
		else null end);
end;
$$ language plpgsql;
