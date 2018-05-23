the data that populates the database is in the "data" folder

the database should be named "data.db"

there is also a backup database in "data" folder, which is the 
same as data.db when you first deploy the application

use create.sql to create schemas.

use .csv files in "data" folder to populate the database.
in case you forget how to use sqlite3 to read .csv files:
	>>>.mode csv
	>>>.import data/<table_name>.csv <table_name>

please remember to create indexes according to report.pdf if you need
