/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: che.jiang <che.jiang@artinchip.com>
 */

#include <rtthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dfs_posix.h>
#include "sqlite3.h"
#include "dbhelper.h"

#define DBG_ENABLE
#define DBG_SECTION_NAME "app.recipe"
#define DBG_LEVEL        DBG_INFO
#define DBG_COLOR
#include <rtdbg.h>

#define RECIPE_DB_NAME "/data/recipe.db"
#define RECIPE_DB_LOG  1

/*Insert nonquery mode have better performance*/
#define RECIPE_DB_INSERT_NONQUERY 1

struct category_entity {
    char id[32];
    char name[32];
    char language[16];
    char image_url[128];
    int is_top;
    int is_home;
    int create_time;
    rt_list_t list;
};
typedef struct category_entity category_entity_t;

struct recipe_entity {
    char recipeId[32];
    int recipe_type;
    int collect;
    char recipe_name[32];
    char recipe_number[8];
    char categorylds[8];
    int flag;
    int create_time;

    rt_list_t list;
};
typedef struct recipe_entity recipe_entity_t;

#if RECIPE_DB_INSERT_NONQUERY
static int category_entity_insert_bind(sqlite3_stmt *stmt, int index, void *arg)
{
    int rc = 0;
    rt_list_t *h = arg, *pos, *n;
    category_entity_t *s = RT_NULL;
    rt_list_for_each_safe(pos, n, h)
    {
        s = rt_list_entry(pos, category_entity_t, list);
        sqlite3_reset(stmt);
        sqlite3_bind_text(stmt, 1, s->id, strlen(s->id), NULL);
        sqlite3_bind_text(stmt, 2, s->name, strlen(s->name), NULL);
        sqlite3_bind_text(stmt, 3, s->language, strlen(s->language), NULL);
        sqlite3_bind_text(stmt, 4, s->image_url, strlen(s->image_url), NULL);
        sqlite3_bind_int(stmt, 5, s->is_top);
        sqlite3_bind_int(stmt, 6, s->is_home);
        sqlite3_bind_int(stmt, 7, s->create_time);
        rc = sqlite3_step(stmt);
    }

    if (rc != SQLITE_DONE)
        return rc;
    return SQLITE_OK;
}
#else
static int category_entity_insert_creat(sqlite3_stmt *stmt, void *arg)
{
    int rc = 0;
    rt_list_t *h = arg, *pos, *n;
    category_entity_t *s = RT_NULL;
    rt_list_for_each_safe(pos, n, h)
    {
        s = rt_list_entry(pos, category_entity_t, list);
        sqlite3_reset(stmt);
        sqlite3_bind_text(stmt, 0, s->id, strlen(s->id), NULL);
        sqlite3_bind_text(stmt, 1, s->name, strlen(s->name), NULL);
        sqlite3_bind_text(stmt, 2, s->language, strlen(s->language), NULL);
        sqlite3_bind_text(stmt, 3, s->image_url, strlen(s->image_url), NULL);
        sqlite3_bind_int(stmt, 4, s->is_top);
        sqlite3_bind_int(stmt, 5, s->is_home);
        sqlite3_bind_int(stmt, 6, s->create_time);
        rc = sqlite3_step(stmt);
    }

    if (rc != SQLITE_DONE)
        return rc;
    return SQLITE_OK;
}
#endif
int category_entity_add(rt_list_t *h)
{
#if RECIPE_DB_INSERT_NONQUERY
    return db_nonquery_operator(
        "insert into CategoryEntity(id,name,language,imageUrl,"
        "isTop,isHome,createTime) values (?,?,?,?,?,?,?);",
        category_entity_insert_bind, h);
#else
    return db_query_by_varpara(
        "insert into CategoryEntity(id,name,language,imageUrl,"
        "isTop,isHome,createTime) values (?,?,?,?,?,?,?);",
        category_entity_insert_creat, h, NULL);
#endif
}

int category_entity_del(int id)
{
    return db_nonquery_by_varpara("delete from CategoryEntity where id=?;",
                                  "%d", id);
}

int category_entity_del_all(void)
{
    return db_nonquery_operator("delete from CategoryEntity;", 0, 0);
}

void category_entity_free_list(rt_list_t *h)
{
    rt_list_t *head = h, *pos, *n;
    category_entity_t *p = RT_NULL;
    rt_list_for_each_safe(pos, n, head)
    {
        p = rt_list_entry(pos, category_entity_t, list);
        rt_free(p);
    }
    rt_free(head);
}

static int category_entity_create(sqlite3_stmt *stmt, void *arg)
{
    category_entity_t *s = arg;
    int ret = sqlite3_step(stmt);
    if (ret != SQLITE_ROW) {
        return 0;
    } else {
        db_stmt_get_text(stmt, 0, s->id);
        db_stmt_get_text(stmt, 1, s->name);
        db_stmt_get_text(stmt, 2, s->language);
        db_stmt_get_text(stmt, 3, s->image_url);
        s->is_top = db_stmt_get_int(stmt, 4);
        s->is_home = db_stmt_get_int(stmt, 5);
        s->create_time = db_stmt_get_int(stmt, 6);
    }
    return ret;
}

static int category_entity_get_id_tbl(sqlite3_stmt *stmt, void *arg)
{
    int64_t *id_tbl = arg;
    char id_str[32] = { 0 };
    int ret, count = 0;
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_ROW) {
        return 0;
    }
    do {
        memset(id_str, 0, 32);
        db_stmt_get_text(stmt, 0, id_str);
        id_tbl[count] = strtoll(id_str, NULL, 10);
        //printf("id_str:%s\tid[%d]:%lld\n", id_str, count, id_tbl[count]);
        count++;

    } while ((ret = sqlite3_step(stmt)) == SQLITE_ROW);
    return count;
}

int category_entity_get_by_all_id()
{
    int i;
    int res;

    /*search recprds count from CategoryEntity table*/
    int valid_count =
        db_query_count_result("select count(*) from CategoryEntity;");
    if (valid_count <= 0) {
        rt_kprintf("no record count %d\n", valid_count);
        return 0;
    }

#if RECIPE_DB_LOG
    rt_kprintf("sql===> record count %d\n", valid_count);
#endif

    category_entity_t *s = rt_calloc(sizeof(category_entity_t), 1);
    if (s == NULL) {
        rt_kprintf("malloc category_entity_t  failed\n");
        return -1;
    }

    int64_t *id_tbl = rt_calloc(sizeof(int64_t), valid_count);
    if (id_tbl == NULL) {
        rt_kprintf("malloc id_table count %d failed\n", valid_count);
        rt_free(s);
        return -1;
    }

    /*search all id from CategoryEntity table*/
    res = db_query_by_varpara("select id from CategoryEntity;",
                              category_entity_get_id_tbl, id_tbl, NULL);
    if (res != valid_count) {
        rt_kprintf("search id_table count %d != %d\n", valid_count, res);
        rt_free(s);
        rt_free(id_tbl);
        return -1;
    }

    /*search records from CategoryEntity table by id*/
    for (i = 0; i < valid_count; i++) {
        res = db_query_by_varpara("select * from CategoryEntity where id=?;",
                                  category_entity_create, s, "%l", id_tbl[i]);
        if (res > 0) {
#if RECIPE_DB_LOG
            rt_kprintf("id:%s\t\tname:%s\tlanguage:%s\timageUrl:%s\n", s->id,
                       s->name, s->language, s->image_url);
#endif
        } else {
            rt_kprintf("no record with id:%lld\n", id_tbl[i]);
        }
    }

    rt_free(s);
    rt_free(id_tbl);

    return valid_count;
}

#if RECIPE_DB_LOG
void category_entity_print_list(rt_list_t *q)
{
    category_entity_t *s = NULL;
    for (s = rt_list_entry((q)->next, category_entity_t, list); &s->list != (q);
         s = rt_list_entry(s->list.next, category_entity_t, list)) {
        rt_kprintf("id:%s\t\tname:%s\tlanguage:%s\timageUrl:%s\n", s->id,
                   s->name, s->language, s->image_url);
    }
}
#endif

static int category_entity_create_queue(sqlite3_stmt *stmt, void *arg)
{
    rt_list_t *q = arg;
    category_entity_t *s;
    int ret, count = 0;
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_ROW) {
        return 0;
    }
    do {
        s = rt_calloc(sizeof(category_entity_t), 1);
        if (!s) {
            LOG_E("No enough memory!");
            goto __create_entity_fail;
        }
        db_stmt_get_text(stmt, 0, s->id);
        db_stmt_get_text(stmt, 1, s->name);
        db_stmt_get_text(stmt, 2, s->language);
        db_stmt_get_text(stmt, 3, s->image_url);
        s->is_top = db_stmt_get_int(stmt, 4);
        s->is_home = db_stmt_get_int(stmt, 5);
        s->create_time = db_stmt_get_int(stmt, 6);
        rt_list_insert_before(q, &(s->list));
        count++;
    } while ((ret = sqlite3_step(stmt)) == SQLITE_ROW);
    return count;
__create_entity_fail:
    return -1;
}

static int category_entity_get_by_id(char *id)
{
    rt_list_t *h = rt_calloc(sizeof(rt_list_t), 1);
    rt_list_init(h);

    /*search records from CategoryEntity table by language*/
    int ret = db_query_by_varpara("select * from CategoryEntity where id=?;",
                                  category_entity_create_queue, h, "%s", id);
    if (ret >= 0) {
#if RECIPE_DB_LOG
        category_entity_print_list(h);
#endif
    } else {
        LOG_E("Get category_entity information failed!");
    }
    category_entity_free_list(h);
    return ret;
}

static int category_entity_get_by_language()
{
    rt_list_t *h = rt_calloc(sizeof(rt_list_t), 1);
    rt_list_init(h);

    /*search recprds count from CategoryEntity table*/
    const char *sql =
        "select count(*) from CategoryEntity where language='en';";
    int valid_count = db_query_count_result(sql);
    if (valid_count <= 0) {
        rt_kprintf("no record count %d\n", valid_count);
        return 0;
    }
#if RECIPE_DB_LOG
    rt_kprintf("sql===> record count %d\n", valid_count);
#endif
    /*search records from CategoryEntity table by language*/
    int ret =
        db_query_by_varpara("select * from CategoryEntity where language=?;",
                            category_entity_create_queue, h, "%s", "en");
    if (ret >= 0) {
#if RECIPE_DB_LOG
        category_entity_print_list(h);
#endif
    } else {
        LOG_E("Get category_entity information failed!");
    }
    category_entity_free_list(h);
    return valid_count;
}

static int category_entity_get_by_name(char *name)
{
    rt_list_t *h = rt_calloc(sizeof(rt_list_t), 1);
    rt_list_init(h);

    /*search records from CategoryEntity table by language*/
    int ret = db_query_by_varpara("select * from CategoryEntity where name=?;",
                                  category_entity_create_queue, h, "%s", name);
    if (ret >= 0) {
#if RECIPE_DB_LOG
        category_entity_print_list(h);
#endif
    } else {
        LOG_E("Get category_entity information failed!");
    }
    category_entity_free_list(h);
    return ret;
}

static int category_entity_update_bind(sqlite3_stmt *stmt, int index, void *arg)
{
    int rc = 0;
    category_entity_t *s = arg;

    sqlite3_bind_text(stmt, 1, s->name, strlen(s->name), NULL);
    sqlite3_bind_text(stmt, 2, s->image_url, strlen(s->image_url), NULL);
    sqlite3_bind_text(stmt, 3, s->id, strlen(s->id), NULL);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
        return rc;
    return SQLITE_OK;
}

int category_entity_update(category_entity_t *s)
{
    return db_nonquery_operator(
        "update CategoryEntity set name=?,imageUrl=? where id=?;",
        category_entity_update_bind, s);
}

static void category_entity(uint8_t argc, char **argv)
{
    if (argc < 3) {
        rt_kprintf("recipe category_entity examples:\n");
        rt_kprintf(
            "\tcategory_entity add n      ---- add n records to CategoryEntity table\n");
        rt_kprintf(
            "\tcategory_entity del n      ---- add id=n record from CategoryEntity table\n");
        rt_kprintf(
            "\tcategory_entity update n   ---- update id=n record to CategoryEntity table\n");
        rt_kprintf(
            "\tcategory_entity query type ---- query records from CategoryEntity table by type, language(0), name(1), id(other)\n");
        return;
    } else {
        char *cmd = argv[1];
        int rand = 0;

        if (rt_strcmp(cmd, "add") == 0) {
            int i = 0, count = 0;
            if (argc >= 3) {
                count = atol(argv[2]);
            }
            if (count == 0) {
                count = 1;
            }
            rt_tick_t ticks = rt_tick_get();
            rand = ticks;
            rt_list_t *h =
                (rt_list_t *)rt_calloc(1, sizeof(rt_list_t));
            rt_list_init(h);
            for (i = 0; i < count; i++) {
                category_entity_t *s =
                    (category_entity_t *)rt_calloc(
                        1, sizeof(category_entity_t));
                rand += i;
                rand %= 99999;
                s->is_top = 0;
                s->is_home = 1;
                s->create_time = ticks;
                snprintf(s->id, sizeof(s->id), "%d", i);
                snprintf(s->language, sizeof(s->language), "en");
                snprintf(s->image_url, sizeof(s->image_url), "/data/image_%d.jpg", rand);
                snprintf(s->name, sizeof(s->name), "category%d", rand);
                rt_list_insert_before(h, &(s->list));
            }
            int res = category_entity_add(h);
            category_entity_free_list(h);
            if (res != SQLITE_OK) {
                LOG_E("add failed!");
            } else {
                ticks = rt_tick_get() - ticks;
                rt_kprintf("Insert %d record(s): %dms, speed: %dms/record\n",
                           count, ticks * 1000 / RT_TICK_PER_SECOND,
                           ticks * 1000 / RT_TICK_PER_SECOND / count);
            }
        } else if (rt_strcmp(cmd, "del") == 0) {
            if (rt_strcmp(argv[2], "all") == 0) {
                if (category_entity_del_all() == SQLITE_OK) {
                    rt_kprintf("Del all record success!\n");
                } else {
                    rt_kprintf("Del all record failed!\n");
                }
            } else {
                rt_uint32_t id = atol(argv[2]);
                if (category_entity_del(id) == SQLITE_OK) {
                    rt_kprintf("Del record success with id:%d\n", id);
                } else {
                    rt_kprintf("Del record failed with id:%d\n", id);
                }
            }
        } else if (rt_strcmp(cmd, "update") == 0) {
            if (argc >= 5) {
                category_entity_t *s =
                    rt_calloc(sizeof(category_entity_t), 1);
                rt_strncpy(s->id, argv[2], rt_strlen(argv[2]));
                rt_strncpy(s->name, argv[3], rt_strlen(argv[3]));
                rt_strncpy(s->image_url, argv[4], rt_strlen(argv[4]));

                if (category_entity_update(s) == SQLITE_OK) {
                    rt_kprintf("update record name %s success!\n", s->name);
                } else {
                    rt_kprintf("update record failed!\n");
                }
                rt_free(s);
            } else {
                rt_kprintf("category_entity update id name imageUrl!\n");
            }
        } else if (rt_strcmp(cmd, "query") == 0) {
            int invalid_count = 0;
            rt_tick_t ticks = rt_tick_get();

            if (atol(argv[2]) == 0) {
                invalid_count = category_entity_get_by_language();
            } else if (atol(argv[2]) == 1) {
                if (argc > 3) {
                    invalid_count = category_entity_get_by_name(argv[3]);
                }
            } else {
                if (argc > 3) {
                    invalid_count = category_entity_get_by_id(argv[3]);
                } else {
                    invalid_count = category_entity_get_by_all_id();
                }
            }

            ticks = rt_tick_get() - ticks;
            if (invalid_count > 0) {
                rt_kprintf("Query %d record(s): %dms, speed: %dms/record\n",
                           invalid_count, ticks * 1000 / RT_TICK_PER_SECOND,
                           ticks * 1000 / RT_TICK_PER_SECOND / invalid_count);
            }
        }
    }
}
MSH_CMD_EXPORT(category_entity, CategoryEntity table add del query);

static void create_recipe_tbl(int argc, char **argv)
{
    if (argc < 2) {
        rt_kprintf("create recipe tbl cmd:\n");
        rt_kprintf(
            "\tcreate_recipe_tbl  CategoryEntity    --- create recipe.db CategoryEntity table\n");
        rt_kprintf(
            "\tcreate_recipe_tbl  CookedEntity      --- create recipe.db CookedEntity table\n");
        rt_kprintf(
            "\tcreate_recipe_tbl  DictEntity        --- create recipe.db DictEntity table\n");
        rt_kprintf(
            "\tcreate_recipe_tbl  OpEntity          --- create recipe.db OpEntity table\n");
        rt_kprintf(
            "\tcreate_recipe_tbl  RecipeEntity      --- create recipe.db RecipeEntity table\n");
        rt_kprintf(
            "\tcreate_recipe_tbl  RecipeLangEntity  --- create recipe.db RecipeLangEntity table\n");
        rt_kprintf(
            "\tcreate_recipe_tbl  room_master_table --- create recipe.db room_master_table table\n");
        rt_kprintf("\n");
        rt_kprintf(
            "Next step, you can use category_entity cmd for add del query records\n\n");
        return;
    }
    int fd = 0;
    int ret = 0;
    int len = 256;
    char *sql = NULL;

    db_set_name(RECIPE_DB_NAME);
    fd = open(db_get_name(), O_RDONLY);
    rt_kprintf(db_get_name());

    if (db_table_is_exist(argv[1]) > 0) {
        /* there is the table int db.close the db. */
        close(fd);
        LOG_I("The table has already existed!\n");
        return;
    }

    sql = rt_calloc(1, len);
    if (rt_strcmp(argv[1], "CategoryEntity") == 0) {
        snprintf(
            sql, len, 
            "CREATE TABLE CategoryEntity (id TEXT NOT NULL, name TEXT NOT NULL, language TEXT NOT NULL, "
            "imageUrl TEXT NOT NULL, isTop INTEGER NOT NULL, isHome INTEGER NOT NULL, createTime INTEGER NOT NULL);");
    } else if (rt_strcmp(argv[1], "CookedEntity") == 0) {
        snprintf(
            sql, len, 
            "CREATE TABLE CookedEntity (cookedId TEXT NOT NULL, recipeId TEXT NOT NULL, yearMonth TEXT NOT NULL, "
            "cookedTime INTEGER NOT NULL);");
    } else if (rt_strcmp(argv[1], "DictEntity") == 0) {
        snprintf(
            sql, len, 
            "CREATE TABLE DictEntity (code TEXT NOT NULL, value TEXT NOT NULL);");
    } else if (rt_strcmp(argv[1], "OpEntity") == 0) {
        snprintf(
            sql, len, 
            "CREATE TABLE OpEntity (id INTEGER PRIMARY KEY AUTOINCREMENT, recipeId TEXT NOT NULL, requestPara TEXT NOT NULL, "
            "extra TEXT NOT NULL, opType INTEGER NOT NULL, count INTEGER NOT NULL);");
    } else if (rt_strcmp(argv[1], "RecipeEntity") == 0) {
        snprintf(
            sql, len, 
            "CREATE TABLE RecipeEntity (recipeId TEXT NOT NULL, recipeType INTEGER NOT NULL, collect INTEGER NOT NULL, recipeName TEXT NOT NULL,"
            "recipeNumber TEXT NOT NULL, categorylds TEXT NOT NULL, flag INTEGER NOT NULL, createTime INTEGER NOT NULL);");
    } else if (rt_strcmp(argv[1], "RecipeLangEntity") == 0) {
        snprintf(
            sql, len, 
            "CREATE TABLE RecipeLangEntity (recipeId TEXT NOT NULL, language TEXT NOT NULL, recipeName TEXT NOT NULL,"
            "recipeSimple TEXT NOT NULL, recipeDetail TEXT NOT NULL, recipeSize INTEGER NOT NULL);");
    } else if (rt_strcmp(argv[1], "room_master_table") == 0) {
        snprintf(
            sql, len, 
            "CREATE TABLE room_master_table (id INTEGER NOT NULL, identity_hash TEXT NOT NULL);");
    }

    if (rt_strlen(sql) > 0) {
        ret = db_create_database(sql);
        if (ret != SQLITE_OK) {
            LOG_E("%s failed", sql);
        }
    }

    rt_free(sql);
}

MSH_CMD_EXPORT(create_recipe_tbl, create recipeData file);
