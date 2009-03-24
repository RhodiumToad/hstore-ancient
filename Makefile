
MODULE_big = hstore-new
OBJS = hstore_io.o hstore_op.o hstore_gist.o hstore_gin.o crc32.o

DATA_built = hstore-new.sql
DATA = uninstall_hstore-new.sql
REGRESS = hstore
DOCS = README.hstore-new

PGXS = $(shell pg_config --pgxs)
include $(PGXS)
