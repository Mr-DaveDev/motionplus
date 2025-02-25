/*
 *    This file is part of MotionPlus.
 *
 *    MotionPlus is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    MotionPlus is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with MotionPlus.  If not, see <https://www.gnu.org/licenses/>.
 *
 *    Copyright 2020 MotionMrDave@gmail.com
 */


#include "motionplus.hpp"
#include "conf.hpp"
#include "util.hpp"
#include "logger.hpp"
#include "dbse.hpp"

static int dbse_global_edits(struct ctx_cam **cam_list)
{

    int retcd = 0;

    if (cam_list[0]->conf->database_dbname == ""){
        MOTION_LOG(ERR, TYPE_DB, NO_ERRNO
                ,_("Invalid database name"));
        retcd = -1;
    }
    if ((((cam_list[0]->conf->database_type == "mysql")) ||
         ((cam_list[0]->conf->database_type == "mariadb")) ||
         ((cam_list[0]->conf->database_type == "pgsql"))) &&
        (cam_list[0]->conf->database_port == 0)){
        MOTION_LOG(ERR, TYPE_DB, NO_ERRNO
                ,_("Must specify database port for mysql/mariadb/pgsql"));
        retcd = -1;
    }

    if (retcd == -1){
        MOTION_LOG(ERR, TYPE_DB, NO_ERRNO
                ,_("Database functionality disabled."));
        cam_list[0]->conf->database_type = "";
    }

    return retcd;

}

void dbse_global_deinit(struct ctx_motapp *motapp)
{

    int indx;

    #if defined(HAVE_MYSQL)
        if (motapp->cam_list[0]->conf->database_type != "") {
            if ((cam_list[0]->conf->database_type == "mysql")) {
                MOTION_LOG(DBG, TYPE_ALL, NO_ERRNO, _("Closing MYSQL"));
                mysql_library_end();
            }
        }
    #else
        (void)motapp;
    #endif /* HAVE_MYSQL */

    #if defined(HAVE_MARIADB)
        if (motapp->cam_list[0]->conf->database_type != "") {
            if ((motapp->cam_list[0]->conf->database_type == "mariadb")) {
                MOTION_LOG(DBG, TYPE_ALL, NO_ERRNO, _("Closing MYSQL"));
                mysql_library_end();
            }
        }
    #else
        (void)motapp;
    #endif /* HAVE_MYSQL */

    indx = 0;
    while (motapp->cam_list[indx] != NULL){
        if (motapp->cam_list[indx]->dbse != NULL) {
            free(motapp->cam_list[indx]->dbse);
        }
        motapp->cam_list[indx]->dbse = NULL;
        indx++;
    }

}

void dbse_global_init(struct ctx_motapp *motapp)
{
    int indx;

    indx = 0;
    while (motapp->cam_list[indx] != NULL) {
        motapp->cam_list[indx]->dbse = (struct ctx_dbse *)mymalloc(sizeof(struct ctx_dbse));
        indx++;
    }

    if (motapp->cam_list[0]->conf->database_type != "") {
        if (dbse_global_edits(motapp->cam_list) == -1) return;

        MOTION_LOG(DBG, TYPE_DB, NO_ERRNO,_("Initializing database"));
        /* Initialize all the database items */
        #if defined(HAVE_MYSQL)
            if ((motapp->cam_list[0]->conf->database_type == "mysql")) {
                if (mysql_library_init(0, NULL, NULL)) {
                    MOTION_LOG(ERR, TYPE_DB, NO_ERRNO
                        ,_("Could not initialize database %s")
                        ,motapp->cam_list[0]->conf->database_type.c_str());
                    motapp->cam_list[0]->conf->database_type = "";
                    return;
                }
            }
        #endif /* HAVE_MYSQL */

        #if defined(HAVE_MARIADB)
            if ((motapp->cam_list[0]->conf->database_type == "mariadb")) {
                if (mysql_library_init(0, NULL, NULL)) {
                    MOTION_LOG(ERR, TYPE_DB, NO_ERRNO
                        ,_("Could not initialize database %s")
                        ,motapp->cam_list[0]->conf->database_type.c_str());
                    motapp->cam_list[0]->conf->database_type = "";
                    return;
                }
            }
        #endif /* HAVE_MARIADB */

        #ifdef HAVE_SQLITE3
            /* database_sqlite3 == NULL if not changed causes each thread to create their own
            * sqlite3 connection this will only happens when using a non-threaded sqlite version */
            motapp->cam_list[0]->dbse->database_sqlite3=NULL;
            if ((motapp->cam_list[0]->conf->database_type == "sqlite3") &&
                (motapp->cam_list[0]->conf->database_dbname != "")) {
                MOTION_LOG(NTC, TYPE_DB, NO_ERRNO
                    ,_("SQLite3 Database filename %s")
                    ,motapp->cam_list[0]->conf->database_dbname.c_str());

                int thread_safe = sqlite3_threadsafe();
                if (thread_safe > 0) {
                    MOTION_LOG(NTC, TYPE_DB, NO_ERRNO, _("SQLite3 is threadsafe"));
                    MOTION_LOG(NTC, TYPE_DB, NO_ERRNO, _("SQLite3 serialized %s")
                        ,(sqlite3_config(SQLITE_CONFIG_SERIALIZED)?_("FAILED"):_("SUCCESS")));
                    if (sqlite3_open(motapp->cam_list[0]->conf->database_dbname.c_str()
                        , &motapp->cam_list[0]->dbse->database_sqlite3) != SQLITE_OK) {
                        MOTION_LOG(ERR, TYPE_DB, NO_ERRNO
                            ,_("Can't open database %s : %s")
                            ,motapp->cam_list[0]->conf->database_dbname.c_str()
                            ,sqlite3_errmsg(motapp->cam_list[0]->dbse->database_sqlite3));
                        sqlite3_close(motapp->cam_list[0]->dbse->database_sqlite3);
                        MOTION_LOG(ERR, TYPE_DB, NO_ERRNO
                            ,_("Could not initialize database %s")
                            ,motapp->cam_list[0]->conf->database_dbname.c_str());
                        motapp->cam_list[0]->conf->database_type = "";
                        return;
                    }
                    MOTION_LOG(NTC, TYPE_DB, NO_ERRNO,_("database_busy_timeout %d msec"),
                            motapp->cam_list[0]->conf->database_busy_timeout);
                    if (sqlite3_busy_timeout(motapp->cam_list[0]->dbse->database_sqlite3
                        , motapp->cam_list[0]->conf->database_busy_timeout) != SQLITE_OK)
                        MOTION_LOG(ERR, TYPE_DB, NO_ERRNO,_("database_busy_timeout failed %s")
                            ,sqlite3_errmsg(motapp->cam_list[0]->dbse->database_sqlite3));
                }
            }
            /* Cascade to all threads */
            indx = 1;
            while (motapp->cam_list[indx] != NULL) {
                motapp->cam_list[indx]->dbse->database_sqlite3 = motapp->cam_list[0]->dbse->database_sqlite3;
                indx++;
            }

        #endif /* HAVE_SQLITE3 */
    }
}

static void dbse_init_mysql(struct ctx_cam *cam)
{

    #if defined(HAVE_MYSQL)
        // close database to be sure that we are not leaking
        mysql_close(cam->dbse->database_mysql);
        cam->dbse->database_event_id = 0;

        cam->dbse->database_mysql = (MYSQL *) mymalloc(sizeof(MYSQL));
        mysql_init(cam->dbse->database_mysql);

        if (!mysql_real_connect(cam->dbse->database_mysql
            , cam->conf->database_host.c_str(), cam->conf->database_user.c_str()
            , cam->conf->database_password.c_str(), cam->conf->database_dbname.c_str()
            , cam->conf->database_port, NULL, 0)) {

            MOTION_LOG(ERR, TYPE_DB, NO_ERRNO
                ,_("Cannot connect to MySQL database %s on host %s with user %s")
                ,cam->conf->database_dbname.c_str(), cam->conf->database_host.c_str()
                ,cam->conf->database_user.c_str());
            MOTION_LOG(ERR, TYPE_DB, NO_ERRNO
                ,_("MySQL error was %s"), mysql_error(cam->dbse->database_mysql));
            MOTION_LOG(ERR, TYPE_DB, NO_ERRNO
                ,_("Disabling database functionality"));
            dbse_global_deinit(cam->cam_list);
            cam->conf->database_type = "";
            return;
        }
        #if (defined(MYSQL_VERSION_ID)) && (MYSQL_VERSION_ID > 50012)
            my_bool my_true = TRUE;
            mysql_options(cam->dbse->database_mysql, MYSQL_OPT_RECONNECT, &my_true);
        #endif
    #else
        (void)cam;  /* Avoid compiler warnings */
    #endif /* HAVE_MYSQL */

    return;

}

static void dbse_init_mariadb(struct ctx_cam *cam)
{

    #if defined(HAVE_MARIADB)
        // close database to be sure that we are not leaking
        mysql_close(cam->dbse->database_mariadb);
        cam->dbse->database_event_id = 0;

        cam->dbse->database_mariadb = (MYSQL *) mymalloc(sizeof(MYSQL));
        mysql_init(cam->dbse->database_mariadb);

        if (!mysql_real_connect(cam->dbse->database_mariadb
            , cam->conf->database_host.c_str(), cam->conf->database_user.c_str()
            , cam->conf->database_password.c_str(), cam->conf->database_dbname.c_str()
            , cam->conf->database_port, NULL, 0)) {
            MOTION_LOG(ERR, TYPE_DB, NO_ERRNO
                ,_("Cannot connect to MySQL database %s on host %s with user %s")
                ,cam->conf->database_dbname.c_str(), cam->conf->database_host.c_str()
                ,cam->conf->database_user.c_str());
            MOTION_LOG(ERR, TYPE_DB, NO_ERRNO
                ,_("MySQL error was %s"), mysql_error(cam->dbse->database_mariadb));
            MOTION_LOG(ERR, TYPE_DB, NO_ERRNO
                ,_("Disabling database functionality"));
            dbse_global_deinit(cam->motapp);
            cam->conf->database_type = "";
            return;
        }
        #if (defined(MYSQL_VERSION_ID)) && (MYSQL_VERSION_ID > 50012)
            my_bool my_true = TRUE;
            mysql_options(cam->dbse->database_mariadb, MYSQL_OPT_RECONNECT, &my_true);
        #endif
    #else
        (void)cam;  /* Avoid compiler warnings */
    #endif /* HAVE_MARIADB */

    return;

}

static void dbse_init_sqlite3(struct ctx_cam *cam)
{
    #ifdef HAVE_SQLITE3
        if (cam->motapp->cam_list[0]->dbse->database_sqlite3 != 0) {
            MOTION_LOG(NTC, TYPE_DB, NO_ERRNO,_("SQLite3 using shared handle"));
            cam->dbse->database_sqlite3 = cam->motapp->cam_list[0]->dbse->database_sqlite3;
        } else {
            MOTION_LOG(NTC, TYPE_DB, NO_ERRNO
                ,_("SQLite3 Database filename %s"), cam->conf->database_dbname.c_str());
            if (sqlite3_open(cam->conf->database_dbname.c_str(), &cam->dbse->database_sqlite3) != SQLITE_OK) {
                MOTION_LOG(ERR, TYPE_DB, NO_ERRNO
                    ,_("Can't open database %s : %s")
                    ,cam->conf->database_dbname.c_str(), sqlite3_errmsg(cam->dbse->database_sqlite3));
                sqlite3_close(cam->dbse->database_sqlite3);
                MOTION_LOG(ERR, TYPE_DB, NO_ERRNO
                    ,_("Disabling database functionality"));
                cam->conf->database_type = "";
                return;
            }
            MOTION_LOG(NTC, TYPE_DB, NO_ERRNO
                ,_("database_busy_timeout %d msec"), cam->conf->database_busy_timeout);
            if (sqlite3_busy_timeout(cam->dbse->database_sqlite3, cam->conf->database_busy_timeout) != SQLITE_OK)
                MOTION_LOG(ERR, TYPE_DB, NO_ERRNO
                    ,_("database_busy_timeout failed %s")
                    ,sqlite3_errmsg(cam->dbse->database_sqlite3));
        }
    #else
        (void)cam;  /* Avoid compiler warnings */
    #endif /* HAVE_SQLITE3 */

    return;

}

static void dbse_init_pgsql(struct ctx_cam *cam)
{
    #ifdef HAVE_PGSQL
        char connstring[255];
        /* Create the connection string.
         * Quote the values so we can have null values (blank)
        */
        snprintf(connstring, 255,
                    "dbname='%s' host='%s' user='%s' password='%s' port='%d'",
                    cam->conf->database_dbname.c_str(), /* dbname */
                    (cam->conf->database_host=="" ? cam->conf->database_host.c_str() : ""), /* host (may be blank) */
                    (cam->conf->database_user=="" ? cam->conf->database_user.c_str() : ""), /* user (may be blank) */
                    (cam->conf->database_password=="" ? cam->conf->database_password.c_str() : ""), /* password (may be blank) */
                    cam->conf->database_port
        );

        cam->dbse->database_pg = PQconnectdb(connstring);
        if (PQstatus(cam->dbse->database_pg) == CONNECTION_BAD) {
            MOTION_LOG(ERR, TYPE_DB, NO_ERRNO
            ,_("Connection to PostgreSQL database '%s' failed: %s")
            ,cam->conf->database_dbname.c_str(), PQerrorMessage(cam->dbse->database_pg));
            MOTION_LOG(ERR, TYPE_DB, NO_ERRNO
                ,_("Disabling database functionality"));
            cam->conf->database_type = "";
            return;
        }
    #else
        (void)cam;  /* Avoid compiler warnings */
    #endif /* HAVE_PGSQL */

    return;
}

void dbse_init(struct ctx_cam *cam)
{

    if (cam->conf->database_type != "") {
        MOTION_LOG(NTC, TYPE_DB, NO_ERRNO
            ,_("Database backend %s"), cam->conf->database_type.c_str());
        if (cam->conf->database_type == "mysql") {
            dbse_init_mysql(cam);
        } else if (cam->conf->database_type == "mariadb") {
            dbse_init_mariadb(cam);
        } else if (cam->conf->database_type == "postgresql") {
            dbse_init_pgsql(cam);
        } else if (cam->conf->database_type == "sqlite3") {
            dbse_init_sqlite3(cam);
        } else {
            MOTION_LOG(NTC, TYPE_DB, NO_ERRNO
            ,_("Invalid Database backend %s")
            , cam->conf->database_type.c_str());
        }
        dbse_sqlmask_update(cam);
    }

    return;
}

void dbse_deinit(struct ctx_cam *cam)
{
    if (cam->conf->database_type != "") {
        MOTION_LOG(DBG, TYPE_ALL, NO_ERRNO, _("Closing database"));

        #if defined(HAVE_MYSQL)
            if (cam->conf->database_type == "mysql") {
                mysql_close(cam->dbse->database_mysql);
                cam->dbse->database_event_id = 0;
            }
        #endif /* HAVE_MYSQL */

        #if defined(HAVE_MARIADB)
            if (cam->conf->database_type == "mariadb") {
                mysql_close(cam->dbse->database_mariadb);
                cam->dbse->database_event_id = 0;
            }
        #endif /* HAVE_MYSQL */

        #ifdef HAVE_PGSQL
            if (cam->conf->database_type == "postgresql") {
                PQfinish(cam->dbse->database_pg);
            }
        #endif /* HAVE_PGSQL */

        #ifdef HAVE_SQLITE3
            /* Close the SQLite database */
            if (cam->conf->database_type == "sqlite3") {
                sqlite3_close(cam->dbse->database_sqlite3);
                cam->dbse->database_sqlite3 = NULL;
            }
        #endif /* HAVE_SQLITE3 */
    }

}

void dbse_sqlmask_update(struct ctx_cam *cam)
{
    /*
    * Set the sql mask file according to the SQL config options
    * We update it for every frame in case the config was updated
    * via remote control.
    */
    cam->dbse->sql_mask = cam->conf->sql_log_picture * (FTYPE_IMAGE + FTYPE_IMAGE_MOTION) +
                    cam->conf->sql_log_snapshot * FTYPE_IMAGE_SNAPSHOT +
                    cam->conf->sql_log_movie * (FTYPE_MPEG + FTYPE_MPEG_MOTION) +
                    cam->conf->sql_log_timelapse * FTYPE_MPEG_TIMELAPSE;

}

static void dbse_mysql_exec(char *sqlquery,struct ctx_cam *cam, int save_id)
{

    #if defined(HAVE_MYSQL)
        MOTION_LOG(DBG, TYPE_DB, NO_ERRNO, "Executing mysql query");
        if (mysql_query(cam->dbse->database_mysql, sqlquery) != 0) {
            int error_code = mysql_errno(cam->dbse->database_mysql);

            MOTION_LOG(ERR, TYPE_DB, SHOW_ERRNO
                ,_("Mysql query failed %s error code %d")
                ,mysql_error(cam->dbse->database_mysql), error_code);
            /* Try to reconnect ONCE if fails continue and discard this sql query */
            if (error_code >= 2000) {
                // Close connection before start a new connection
                mysql_close(cam->dbse->database_mysql);

                cam->dbse->database_mysql = (MYSQL *) mymalloc(sizeof(MYSQL));
                mysql_init(cam->dbse->database_mysql);

                if (!mysql_real_connect(cam->dbse->database_mysql, cam->conf->database_host.c_str(),
                                        cam->conf->database_user.c_str(), cam->conf->database_password.c_str(),
                                        cam->conf->database_dbname.c_str(), cam->conf->database_port, NULL, 0)) {
                    MOTION_LOG(ALR, TYPE_DB, NO_ERRNO
                        ,_("Cannot reconnect to MySQL"
                        " database %s on host %s with user %s MySQL error was %s"),
                        cam->conf->database_dbname.c_str(),
                        cam->conf->database_host.c_str(), cam->conf->database_user.c_str(),
                        mysql_error(cam->dbse->database_mysql));
                } else {
                    MOTION_LOG(INF, TYPE_DB, NO_ERRNO
                        ,_("Re-Connection to Mysql database '%s' Succeed")
                        ,cam->conf->database_dbname.c_str());
                    if (mysql_query(cam->dbse->database_mysql, sqlquery) != 0) {
                        int error_my = mysql_errno(cam->dbse->database_mysql);
                        MOTION_LOG(ERR, TYPE_DB, SHOW_ERRNO
                            ,_("after re-connection Mysql query failed %s error code %d")
                            ,mysql_error(cam->dbse->database_mysql), error_my);
                    }
                }
            }
        }
        if (save_id) {
            cam->dbse->database_event_id = (unsigned long long) mysql_insert_id(cam->dbse->database_mysql);
        }
    #else
        (void)sqlquery;
        (void)cam;
        (void)save_id;
    #endif /* HAVE_MYSQL  HAVE_MARIADB*/

}

static void dbse_mariadb_exec(char *sqlquery,struct ctx_cam *cam, int save_id)
{

    #if defined(HAVE_MARIADB)
        MOTION_LOG(DBG, TYPE_DB, NO_ERRNO, "Executing mysql query");
        if (mysql_query(cam->dbse->database_mariadb, sqlquery) != 0) {
            int error_code = mysql_errno(cam->dbse->database_mariadb);

            MOTION_LOG(ERR, TYPE_DB, SHOW_ERRNO
                ,_("Mysql query failed %s error code %d")
                ,mysql_error(cam->dbse->database_mariadb), error_code);
            /* Try to reconnect ONCE if fails continue and discard this sql query */
            if (error_code >= 2000) {
                // Close connection before start a new connection
                mysql_close(cam->dbse->database_mariadb);

                cam->dbse->database_mariadb = (MYSQL *) mymalloc(sizeof(MYSQL));
                mysql_init(cam->dbse->database_mariadb);

                if (!mysql_real_connect(cam->dbse->database_mariadb, cam->conf->database_host.c_str(),
                                        cam->conf->database_user.c_str(), cam->conf->database_password.c_str(),
                                        cam->conf->database_dbname.c_str(),cam->conf->database_port, NULL, 0)) {
                    MOTION_LOG(ALR, TYPE_DB, NO_ERRNO
                        ,_("Cannot reconnect to MySQL"
                        " database %s on host %s with user %s MySQL error was %s"),
                        cam->conf->database_dbname.c_str(),
                        cam->conf->database_host.c_str(), cam->conf->database_user.c_str(),
                        mysql_error(cam->dbse->database_mariadb));
                } else {
                    MOTION_LOG(INF, TYPE_DB, NO_ERRNO
                        ,_("Re-Connection to Mysql database '%s' Succeed")
                        ,cam->conf->database_dbname.c_str());
                    if (mysql_query(cam->dbse->database_mariadb, sqlquery) != 0) {
                        int error_my = mysql_errno(cam->dbse->database_mariadb);
                        MOTION_LOG(ERR, TYPE_DB, SHOW_ERRNO
                            ,_("after re-connection Mysql query failed %s error code %d")
                            ,mysql_error(cam->dbse->database_mariadb), error_my);
                    }
                }
            }
        }
        if (save_id) {
            cam->dbse->database_event_id = (unsigned long long) mysql_insert_id(cam->dbse->database_mariadb);
        }
    #else
        (void)sqlquery;
        (void)cam;
        (void)save_id;
    #endif /* HAVE_MARIADB*/

}

static void dbse_pgsql_exec(char *sqlquery,struct ctx_cam *cam, int save_id)
{
    #ifdef HAVE_PGSQL
        MOTION_LOG(DBG, TYPE_DB, NO_ERRNO, "Executing postgresql query");
        PGresult *res;

        res = PQexec(cam->dbse->database_pg, sqlquery);

        if (PQstatus(cam->dbse->database_pg) == CONNECTION_BAD) {

            MOTION_LOG(ERR, TYPE_DB, NO_ERRNO
                ,_("Connection to PostgreSQL database '%s' failed: %s")
                ,cam->conf->database_dbname.c_str(), PQerrorMessage(cam->dbse->database_pg));

        // This function will close the connection to the server and attempt to reestablish a new connection to the same server,
        // using all the same parameters previously used. This may be useful for error recovery if a working connection is lost
            PQreset(cam->dbse->database_pg);

            if (PQstatus(cam->dbse->database_pg) == CONNECTION_BAD) {
                MOTION_LOG(ERR, TYPE_DB, NO_ERRNO
                    ,_("Re-Connection to PostgreSQL database '%s' failed: %s")
                    ,cam->conf->database_dbname.c_str(), PQerrorMessage(cam->dbse->database_pg));
            } else {
                MOTION_LOG(INF, TYPE_DB, NO_ERRNO
                    ,_("Re-Connection to PostgreSQL database '%s' Succeed")
                    ,cam->conf->database_dbname.c_str());
            }

        } else if (!(PQresultStatus(res) == PGRES_COMMAND_OK || PQresultStatus(res) == PGRES_TUPLES_OK)) {
            MOTION_LOG(ERR, TYPE_DB, SHOW_ERRNO, "PGSQL query failed: [%s]  %s %s",
                    sqlquery, PQresStatus(PQresultStatus(res)), PQresultErrorMessage(res));
        }
        if (save_id) {
            //ToDO:  Find the equivalent option for pgsql
            cam->dbse->database_event_id = 0;
        }

        PQclear(res);
    #else
        (void)sqlquery;
        (void)cam;
        (void)save_id;
    #endif /* HAVE_PGSQL */

}

static void dbse_sqlite3_exec(char *sqlquery,struct ctx_cam *cam, int save_id)
{
    #ifdef HAVE_SQLITE3
        int res;
        char *errmsg = 0;
        MOTION_LOG(DBG, TYPE_DB, NO_ERRNO, "Executing sqlite query");
        res = sqlite3_exec(cam->dbse->database_sqlite3, sqlquery, NULL, 0, &errmsg);
        if (res != SQLITE_OK ) {
            MOTION_LOG(ERR, TYPE_DB, NO_ERRNO, _("SQLite error was %s"), errmsg);
            sqlite3_free(errmsg);
        }
        if (save_id) {
            //ToDO:  Find the equivalent option for sqlite3
            cam->dbse->database_event_id = 0;
        }
    #else
        (void)sqlquery;
        (void)cam;
        (void)save_id;
    #endif /* HAVE_SQLITE3 */
}

void dbse_firstmotion(struct ctx_cam *cam)
{

    char sqlquery[PATH_MAX];

    mystrftime(cam, sqlquery, sizeof(sqlquery), cam->conf->sql_query_start.c_str(),
                &cam->current_image->imgts, NULL, 0);

    if (strlen(sqlquery) <= 0) {
        MOTION_LOG(WRN, TYPE_DB, NO_ERRNO, "Ignoring empty sql query");
        return;
    }

    if (cam->conf->database_type == "mysql") {
        dbse_mysql_exec(sqlquery, cam, 1);
    } else if (cam->conf->database_type == "mariadb") {
        dbse_mariadb_exec(sqlquery, cam, 1);
    } else if (cam->conf->database_type == "postgresql") {
        dbse_pgsql_exec(sqlquery, cam, 1);
    } else if (cam->conf->database_type == "sqlite3") {
        dbse_sqlite3_exec(sqlquery, cam, 1);
    }

}

void dbse_newfile(struct ctx_cam *cam, char *filename, int sqltype, struct timespec *ts1)
{
    char sqlquery[PATH_MAX];

    mystrftime(cam, sqlquery, sizeof(sqlquery), cam->conf->sql_query.c_str(),
                ts1, filename, sqltype);

    if (strlen(sqlquery) <= 0) {
        MOTION_LOG(WRN, TYPE_DB, NO_ERRNO, "Ignoring empty sql query");
        return;
    }

    if (cam->conf->database_type == "mysql") {
        dbse_mysql_exec(sqlquery, cam, 0);
    } else if (cam->conf->database_type == "mariadb") {
        dbse_mariadb_exec(sqlquery, cam, 0);
    } else if (cam->conf->database_type == "postgresql") {
        dbse_pgsql_exec(sqlquery, cam, 0);
    } else if (cam->conf->database_type == "sqlite3") {
        dbse_sqlite3_exec(sqlquery, cam, 0);
    }

}

void dbse_fileclose(struct ctx_cam *cam, char *filename, int sqltype, struct timespec *ts1)
{
    char sqlquery[PATH_MAX];

    mystrftime(cam, sqlquery, sizeof(sqlquery), cam->conf->sql_query_stop.c_str(),
                ts1, filename, sqltype);

    if (cam->conf->database_type == "mysql") {
        dbse_mysql_exec(sqlquery, cam, 0);
    } else if (cam->conf->database_type == "mariadb") {
        dbse_mariadb_exec(sqlquery, cam, 0);
    } else if (cam->conf->database_type == "postgresql") {
        dbse_pgsql_exec(sqlquery, cam, 0);
    } else if (cam->conf->database_type == "sqlite3") {
        dbse_sqlite3_exec(sqlquery, cam, 0);
    }

}
