# debug.mk must be declared before dev_mngr.mk, as they are variables
# exported by the former used by the latter
include hal/debug/debug.mk
include hal/ll_io/ll_io.mk
include hal/sm_io/sm_io.mk
include hal/sdb/sdb.mk
include hal/dev_mngr/dev_mngr.mk
include hal/dev_io/dev_io.mk
include hal/msg/msg.mk
include hal/hal_utils/hal_utils.mk
include hal/boards/boards.mk

std_hal_INCLUDE_DIRS = hal/include \
		       hal/include/hw \
		       hal/include/protocols \
		       hal/include/chips

hal_OUT += $(dev_mngr_OUT) $(dev_io_OUT)

# All possible objects. Used for cleaning
hal_all_OUT += $(dev_mngr_all_OUT) $(dev_io_all_OUT)

# For each target in hal_OUT we add the necessary objects
# We need exp_ops_OBJS for hal_utils_OBJS, so we include it here.
dev_mngr_OBJS += $(dev_mngr_core_OBJS) $(debug_OBJS) \
		$(hal_utils_OBJS) $(exp_ops_OBJS) \
		$(thsafe_msg_zmq_OBJS) $(ll_io_utils_OBJS) \
        $(dev_io_utils_OBJS)
# msg_OBJS already contains exp_ops_OBJS. So, there is no need to include
# it here twice
dev_io_OBJS += $(dev_io_core_OBJS) $(ll_io_OBJS) $(sm_io_OBJS) \
	   $(msg_OBJS) $(debug_OBJS) $(hal_utils_OBJS)

dev_mngr_LIBS =
dev_mngr_STATIC_LIBS =

dev_io_LIBS = -lbsmp
dev_io_STATIC_LIBS = $(LIBCLIENT_DIR)/libbpmclient.a

# Merge all hal objects together
hal_OBJS = $(debug_OBJS) \
	   $(hal_utils_OBJS) \
	   $(ll_io_OBJS) \
	   $(sm_io_OBJS) \
	   $(msg_OBJS) \
	   $(dev_mngr_core_OBJS) \
	   $(dev_io_core_OBJS)

# Merge all include directories together
hal_all_INCLUDE_DIRS += $(std_hal_INCLUDE_DIRS) \
	$(debug_INCLUDE_DIRS) \
	$(hal_utils_INCLUDE_DIRS) \
	$(sdb_INCLUDE_DIRS) \
	$(ll_io_INCLUDE_DIRS) \
	$(sm_io_INCLUDE_DIRS) \
	$(msg_INCLUDE_DIRS) \
	$(dev_mngr_INCLUDE_DIRS) \
	$(dev_io_INCLUDE_DIRS) \
	$(boards_INCLUDE_DIRS)

# Change the include dirs into flags
hal_INCLUDE_DIRS = $(addprefix -I, $(hal_all_INCLUDE_DIRS))
