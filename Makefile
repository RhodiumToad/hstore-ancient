
MODULE_big = hstore-new
OBJS = hstore_io.o hstore_op.o hstore_gist.o hstore_gin.o hstore_compat.o crc32.o

DATA_built = hstore-new.sql
DATA = uninstall_hstore-new.sql
REGRESS = hstore
DOCS = README.hstore-new

PG_CPPFLAGS = -DHSTORE_IS_HSTORE_NEW

PG_CONFIG = pg_config
PGXS = $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
