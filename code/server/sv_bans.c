
#include "server.h"
#include "../sqlite3/sqlite3.h"

const char *schema = "CREATE TABLE IF NOT EXISTS `bans` (`id` INTEGER NULL DEFAULT NULL, `ip` MEDIUMTEXT(16) NULL DEFAULT NULL,`expire` INTEGER(32) NULL DEFAULT -1,PRIMARY KEY (`id`));";