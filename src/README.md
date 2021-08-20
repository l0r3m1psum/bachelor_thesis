# Istruzioni di compilazione

Usare semplicemente il comando `make`

# Esecuzione

`make` genererà un file eseguibile chiamato `main` questo prende come argomento
un file CSV con 8 colonne e ne stamperà le prime 5 righe come test della lettura
dei file CSV appunto.

# Importazione file nella base di dati

Per importare i dati nella base di dati sarà necessario creare lo schema con il
comando `psql -f schema.sql`, che creerà il database "test" poi importarli con
il comando `psql -f import.sql -v geo=<path file geo> -v meteo=<path file meteo>`.
Nel drive i CSV con degli esempi di dati da importare nella base sono nella
cartella `res`.
