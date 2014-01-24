#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "recorder.h"
#include "ams_rec.h"
/* routines needed for debug output */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/output_api.h>

#ifndef true
#define true 1
#endif
#ifndef false
#define false 1
#endif

int doWrite( _req *, int);
int doRead( void *, int);
int doWriteRead(void *, int, _resp *);

int s_ams_rec_class = -1;
int s_ams_rec_handle = -1;

static uint16_t s_sequence = 0;

/* Only available for OS clients (via CHIF) for OS API callers from _service */
int rec_api13( const char* name )
{
    _req req;
    _resp resp;
    int  size;
    REC_13_MSG *msg; 

    DEBUGMSGTL(("rec:rec_log", "rec_api13\n"));
    memset (&req,0,sizeof(_req));
    
    req.hdr.svc = 0x11;
    req.hdr.seq = ++s_sequence;

    req.msg.type = 2;
    req.msg.rec_msg.rec.subtype = REC_API_13;
    msg = &req.msg.rec_msg.rec.cmd.m13;
    strncpy(msg->name, name, sizeof(msg->name) - 1);
    msg->name[sizeof(msg->name) - 1] = 0;
	size = sizeof(pkt_hdr) + sizeof(REC_13_MSG) + 8;
    if (doWrite(&req, size) > 0 )
        if (doRead(&resp,sizeof(_resp)) > 0) {
            return (resp.msg.rcode);
        }
    
    return (-1);
}

/* Register */
int rec_api1(const char * name, 
                unsigned int cls, 
                unsigned int flags, 
                unsigned int size, 
                int * handle )
{
    _resp resp;
    int rc;
    _req req;
    REC_1_MSG *msg;

    DEBUGMSGTL(("rec:rec_log", "rec_api1\n"));
    memset (&req,0,sizeof(_req));

    req.hdr.svc = 0x11;
    req.hdr.seq = ++s_sequence;

    req.msg.type = 2;

    req.msg.rec_msg.rec.subtype = REC_API_1;

    msg = &req.msg.rec_msg.rec.cmd.m1;
    msg->classs = cls;
    msg->flags = flags;
    msg->size = size;
    msg->count = 0;

    strncpy(msg->name, name, sizeof(msg->name) - 1);
    msg->name[sizeof(msg->name) - 1] = 0;

    if ((rc = doWriteRead(&req, sizeof(REC_1_MSG) + 16, &resp)) == 0)
        *handle = resp.msg.rec_msg.returned.handle;
    return (rc);
}

int rec_api1_array(const char * name, 
                      unsigned int cls, 
                      unsigned int flags, 
                      unsigned int size, 
                      unsigned int count, 
                      int * handle )
{
    _resp resp;
    int rc;
    _req req;
    REC_1_MSG *msg;

    memset (&req,0,sizeof(_req));

    req.hdr.svc = 0x11;
    req.hdr.seq = ++s_sequence;

    req.msg.type = 2;

    req.msg.rec_msg.rec.subtype = REC_API_1;

    msg = &req.msg.rec_msg.rec.cmd.m1;
    msg->classs = cls;
    msg->flags = flags;
    msg->size = size;
    msg->count = count;

    strncpy(msg->name, name, sizeof(msg->name) - 1);
    msg->name[sizeof(msg->name) - 1] = 0;

    if ((rc = doWriteRead(&req, sizeof(REC_1_MSG) + 16, &resp)) == 0)
        *handle = resp.msg.rec_msg.returned.handle;
    return (rc);
}

int rec_api2(int handle)
{
    _resp resp;
    int rc;
    _req req;
    REC_2_MSG *msg;

    memset (&req,0,sizeof(_req));

    req.hdr.svc = 0x11;
    req.hdr.seq = ++s_sequence;

    req.msg.type = 2;

    req.msg.rec_msg.rec.subtype = REC_API_2;

    msg = &req.msg.rec_msg.rec.cmd.m2;
    msg->handle = handle;

    if ((rc = doWriteRead(&req, sizeof(REC_2_MSG) + 16, &resp)) == 0) {
        handle = resp.msg.rec_msg.returned.handle;
        DEBUGMSGTL(("rec:rec_log", "handle = %x\n", handle));
    }
    return (rc);
}

static int _rec_api4(int handle, 
                        unsigned int size, 
                        int d_type,
                        const descript * toc )
{
    _resp resp;
    int rc;
    _req req;
    REC_4_MSG *msg;

    memset (&req,0,sizeof(_req));

    req.hdr.svc = 0x11;
    req.hdr.seq = ++s_sequence;

    req.msg.type = 2;

    req.msg.rec_msg.rec.subtype = REC_API_4;

    msg = &req.msg.rec_msg.rec.cmd.m4;
    msg->handle = handle;
    msg->size = size;
    msg->selection = d_type;
    memcpy(&msg->toc, toc, size);

    if ((rc = doWriteRead(&req, 16 + size + (3 * sizeof(int)) ,&resp)) == 0) {
        handle = resp.msg.rec_msg.returned.handle;
    }
    return (rc);
}

int rec_api4_class(int handle, 
                        unsigned int size, 
                        const descript * toc )
{
    return (_rec_api4(handle, size, REC_DESCRIBE_CLASS, toc));
}

int rec_api4_code(int handle, 
                        unsigned int size, 
                        const descript * toc )
{
    return (_rec_api4(handle, size, REC_DESCRIBE_CODE, toc));
}

int rec_api4_field(int handle, 
                        unsigned int size, 
                        const descript * toc )
{
    DEBUGMSGTL(("rec:rec_log", "rec_api4_field\n"));
    return (_rec_api4(handle, size, REC_DESCRIBE_FIELD, toc));
}

int rec_api3(int handle, unsigned int code)
{
    _resp resp;
    int rc;
    _req req;
    REC_3_MSG *msg;

    DEBUGMSGTL(("rec:rec_log", "rec_api3\n"));
    memset (&req,0,sizeof(_req));

    req.hdr.svc = 0x11;
    req.hdr.seq = ++s_sequence;

    req.msg.type = 2;

    req.msg.rec_msg.rec.subtype = REC_API_3;

    msg = &req.msg.rec_msg.rec.cmd.m3;
    msg->handle = handle;
    msg->code = code;
    msg->modify_mask = REC_MODIFY_CODE;

    if ((rc = doWriteRead(&req, sizeof(REC_3_MSG) + 16, &resp)) == 0){
        handle = resp.msg.rec_msg.returned.handle;
        DEBUGMSGTL(("rec:rec_log", "handle = %x\n", handle));
    }
    return (rc);
}

int rec_api5(int handle, 
              int type, 
              unsigned int bytes, 
              const UINT32 * f )
{
    _resp resp;
    int rc;
    _req req;
    REC_5_MSG *msg;

    DEBUGMSGTL(("rec:rec_log", "rec_api5\n"));
    memset (&req,0,sizeof(_req));

    req.hdr.svc = 0x11;
    req.hdr.seq = ++s_sequence;

    req.msg.type = 2;

    req.msg.rec_msg.rec.subtype = REC_API_5;

    msg = &req.msg.rec_msg.rec.cmd.m5;
    msg->handle = handle;
    msg->bytes = bytes;
    if (bytes != 0) 
        memcpy(&msg->filter,f,bytes*sizeof(UINT32));

    if ((rc = doWriteRead(&req, sizeof(REC_5_MSG) + 16 + bytes, &resp)) == 0) {
        handle = resp.msg.rec_msg.returned.handle;
        DEBUGMSGTL(("rec:rec_log", "handle = %x\n", handle));
    }
    return (rc);
}

static int _rec_api6( int handle, 
            int log_type,
            int instance,
            int field,
            const char * data, 
            unsigned int size )
{
    _resp resp;
    int rc;
    _req req;
    REC_6_MSG *msg;

    DEBUGMSGTL(("rec:rec_log", "_rec_api6\n"));
    memset(&req,  0,sizeof(_req));
    memset(&resp, 0, sizeof(_resp));

    req.hdr.svc = 0x11;
    req.hdr.seq = ++s_sequence;

    req.msg.type = 2;
    req.msg.rec_msg.rec.subtype = log_type;

    msg = &req.msg.rec_msg.rec.cmd.m6;

    msg->field = field;
    msg->instance = instance;
    msg->handle = handle;
    msg->size = size;
    if (size != 0) 
        memcpy(msg->data, data, size);

    if ((rc = doWriteRead(&req, size + 32, &resp)) == 0) {
        handle = resp.msg.rec_msg.returned.handle;
        DEBUGMSGTL(("rec:rec_log", "handle = %x\n", handle));
    }
    return (rc);
}

int rec_api6( int handle, 
            const char * data, 
            unsigned int size )
{
    DEBUGMSGTL(("rec:rec_log", "rec_api6\n"));
    return (_rec_api6(handle, REC_API_6, 0, 0, data, size));
}

int rec_api6_array(int handle, 
                 const char * data, 
                 unsigned int size )
{
    DEBUGMSGTL(("rec:rec_log", "rec_api6_array\n"));
    return (_rec_api6(handle, REC_API_7, 0, 0, data, size));
}

int rec_api6_instance(int handle, 
                    unsigned int instance, 
                    const char * data, 
                    unsigned int size )
{
    DEBUGMSGTL(("rec:rec_log", "rec_api6_instance\n"));
    return (_rec_api6(handle, REC_API_9, instance, 0, data, size));
}

int rec_api6_field(int handle, 
                    unsigned int instance, 
                    unsigned int field, 
                    const char * data, 
                    unsigned int size )
{
    DEBUGMSGTL(("rec:rec_log", "rec_api6_field\n"));
    return (_rec_api6(handle, REC_API_8, instance, field, data, size));
}

int rec_api6_static(int handle, 
                  const char * data, 
                  unsigned int size )
{
    DEBUGMSGTL(("rec:rec_log", "rec_api6_static\n"));
    return (_rec_api6(handle, REC_API_11, 0, 0, data, size));
}

int rec_init() {
    int rc;
    if (s_ams_rec_class == -1 ) { 
        if ((s_ams_rec_class = rec_api13(REC_CLASS_AMS)) == -1) {
            DEBUGMSGTL(("rec:rec_log_error", "Can't allocate AMS class\n"));
            return false;
        }
    }
    DEBUGMSGTL(("rec:rec_log", "AMS class = %d\n",s_ams_rec_class));

    if (s_ams_rec_handle == -1 ) {
        if ((rc = rec_api1(REC_CLASS_AMS, s_ams_rec_class, 
                  REC_STATUS_NO_COMPARE, RECORDER_MAX_BLOB_SIZE, 
                  &s_ams_rec_handle)) != RECORDER_OK ) {
            DEBUGMSGTL(("rec:rec_log_error", "RegisterRecorderFeeder failed (%d)\n", 
                            rc));
            return false;
        }
    }
    DEBUGMSGTL(("rec:rec_log", "Handle = %x\n", s_ams_rec_handle));
    return true;
}

