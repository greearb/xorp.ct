#ifndef __XORP_BUG_CATCHER_INC__
#define __XORP_BUG_CATCHER_INC__

#include <assert.h>

class BugCatcher {
private:
   unsigned int magic;
   static unsigned int _cnt;

public:
#define X_BUG_CATCHER_MAGIC 0x1234543
   BugCatcher() { magic = X_BUG_CATCHER_MAGIC; _cnt++; }
   BugCatcher(const BugCatcher& rhs) { magic = rhs.magic; _cnt++; }
   virtual ~BugCatcher() { assert_not_deleted(); magic = 0xdeadbeef; _cnt--; }

   virtual void assert_not_deleted() const {
      assert(magic == X_BUG_CATCHER_MAGIC);
   }

   static int get_instance_count() { return _cnt; }
};

#endif
