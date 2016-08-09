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

#define REC_BLACKBOX 2
#ifdef UTEST
void dump_chif(rec_chif_req * req) {
    fprintf(stderr, "pktsz = %d ",req->hdr.pktsz);
    fprintf(stderr, "seq = %d   ",req->hdr.seq);
    fprintf(stderr, "cmd = %d   ",req->hdr.cmd);
    fprintf(stderr, "svc = %d\n",req->hdr.svc);
}

void dump_alloc_req(rec_chif_req* req) {
    fprintf(stderr, "CHIF Request: ");
    dump_chif(req);

    fprintf(stderr, "request->type = %d\n",req->msg.type);
    fprintf(stderr, "request->rec_msg.subtype = %d\n",
				req->msg.rec_msg.bb.subtype);
    fprintf(stderr, "request->rec_msg.cmd.alloc.name = %s\n",
				req->msg.rec_msg.bb.cmd.alloc.name);
}

void dump_alloc_resp(rec_chif_resp* resp) {
    fprintf(stderr, "CHIF Response: ");
    dump_chif(resp);

    fprintf(stderr, "response->rcode = %d\n",resp->msg.rcode);
    fprintf(stderr, "response->rec_msg.returned.handle = %d\n",
				resp->msg.rec_msg.returned.handle);
}

#else
void dump_chif(rec_chif_req* req) {
}
void dump_alloc_req(rec_chif_req* resp) {
}
void dump_alloc_resp(rec_chif_resp* resp) {
}

#endif
int doWrite( rec_chif_req *, int);
int doRead( void *, int);
int doWriteRead(void *, int, rec_chif_resp *);

int s_ams_rec_class = -1;
int s_ams_rec_handle = -1;

static uint16_t s_sequence = 0;

/* Only available for OS clients (via CHIF) for OS API callers from _service */
int rec_api13( const char* name )
{
    rec_chif_req req;
    rec_chif_resp resp;
    int  size;
    REC_ALLOC_MSG *alloc_msg; 

    DEBUGMSGTL(("rec:rec_log", "rec_class_allocate\n"));
    memset (&req,0,sizeof(rec_chif_req));
    
    req.hdr.svc = RECORDER_CHIF_SERVICE;
    req.hdr.seq = ++s_sequence;

    req.msg.type = REC_BLACKBOX;
    req.msg.rec_msg.bb.subtype = REC_API13;
    alloc_msg = &req.msg.rec_msg.bb.cmd.alloc;
    strncpy(alloc_msg->name, name, sizeof(alloc_msg->name) - 1);
    alloc_msg->name[sizeof(alloc_msg->name) - 1] = 0;
	//size = (int)alloc_msg - (int)&req + sizeof(alloc_msg->name) + 4;
	size = sizeof(pkt_hdr) + sizeof(REC_ALLOC_MSG) + 8;
	dump_alloc_req(&req);
    if (doWrite(&req, size) > 0 )
        if (doRead(&resp,sizeof(rec_chif_resp)) > 0) {
	    dump_alloc_resp(&resp);
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
    rec_chif_resp resp;
    int rc;
    rec_chif_req req;
    REC_API1_MSG *register_msg;

    DEBUGMSGTL(("rec:rec_log", "rec_register\n"));
    memset (&req,0,sizeof(rec_chif_req));

    req.hdr.svc = RECORDER_CHIF_SERVICE;
    req.hdr.seq = ++s_sequence;

    req.msg.type = REC_BLACKBOX;

    req.msg.rec_msg.bb.subtype = REC_API1;

    register_msg = &req.msg.rec_msg.bb.cmd.registration;
    register_msg->classs = cls;
    register_msg->flags = flags;
    register_msg->size = size;
    register_msg->count = 0;

    strncpy(register_msg->name, name, sizeof(register_msg->name) - 1);
    register_msg->name[sizeof(register_msg->name) - 1] = 0;

    if ((rc = doWriteRead(&req, sizeof(REC_API1_MSG) + 16, &resp)) == 0)
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
    rec_chif_resp resp;
    int rc;
    rec_chif_req req;
    REC_API1_MSG *register_msg;

    DEBUGMSGTL(("rec:rec_log", "rec_register_array\n"));
    memset (&req,0,sizeof(rec_chif_req));

    req.hdr.svc = RECORDER_CHIF_SERVICE;
    req.hdr.seq = ++s_sequence;

    req.msg.type = REC_BLACKBOX;

    req.msg.rec_msg.bb.subtype = REC_API1;

    register_msg = &req.msg.rec_msg.bb.cmd.registration;
    register_msg->classs = cls;
    register_msg->flags = flags;
    register_msg->size = size;
    register_msg->count = count;

    strncpy(register_msg->name, name, sizeof(register_msg->name) - 1);
    register_msg->name[sizeof(register_msg->name) - 1] = 0;

    if ((rc = doWriteRead(&req, sizeof(REC_API1_MSG) + 16, &resp)) == 0)
        *handle = resp.msg.rec_msg.returned.handle;
    return (rc);
}

int rec_api2(int handle)
{
    rec_chif_resp resp;
    int rc;
    rec_chif_req req;
    REC_API2_MSG *unregister_msg;

    DEBUGMSGTL(("rec:rec_log", "rec_unregister\n"));
    memset (&req,0,sizeof(rec_chif_req));

    req.hdr.svc = RECORDER_CHIF_SERVICE;
    req.hdr.seq = ++s_sequence;

    req.msg.type = REC_BLACKBOX;

    req.msg.rec_msg.bb.subtype = REC_API1;

    unregister_msg = &req.msg.rec_msg.bb.cmd.unregister;
    unregister_msg->handle = handle;

    if ((rc = doWriteRead(&req, sizeof(REC_API2_MSG) + 16, &resp)) == 0) {
        handle = resp.msg.rec_msg.returned.handle;
        DEBUGMSGTL(("rec:rec_log", "handle = %x\n", handle));
    }
    return (rc);
}

static int _rec_api4(int handle, 
                        unsigned int size, 
                        int descriptor_type,
                        const RECORDER_API4_RECORD * toc )
{
    rec_chif_resp resp;
    int rc;
    rec_chif_req req;
    REC_DESCRIBE_MSG *describe_msg;

    DEBUGMSGTL(("rec:rec_log", "_rec_descriptor\n"));
    memset (&req,0,sizeof(rec_chif_req));

    req.hdr.svc = RECORDER_CHIF_SERVICE;
    req.hdr.seq = ++s_sequence;

    req.msg.type = REC_BLACKBOX;

    req.msg.rec_msg.bb.subtype = REC_API4;

    describe_msg = &req.msg.rec_msg.bb.cmd.describe;
    describe_msg->handle = handle;
    describe_msg->size = size;
    describe_msg->selection = descriptor_type;
    memcpy(&describe_msg->toc, toc, size);
    dump_chunk("rec:rec_log","Request Packet", (const u_char *)&req, 
                                             16+size + (3 * sizeof(int)));

    if ((rc = doWriteRead(&req, 16 + size + (3 * sizeof(int)) ,&resp)) == 0) {
        handle = resp.msg.rec_msg.returned.handle;
        DEBUGMSGTL(("rec:rec_log", "handle = %x\n", handle));
    }
    return (rc);
}

int rec_api4_class(int handle, 
                        unsigned int size, 
                        const RECORDER_API4_RECORD * toc )
{
    DEBUGMSGTL(("rec:rec_log", "rec_descriptor_class\n"));
    return (_rec_api4(handle, size, REC_DESCRIBE_CLASS, toc));
}

int rec_api4_code(int handle, 
                        unsigned int size, 
                        const RECORDER_API4_RECORD * toc )
{
    DEBUGMSGTL(("rec:rec_log", "rec_descriptor_code\n"));
    return (_rec_api4(handle, size, REC_DESCRIBE_CODE, toc));
}

int rec_api4_field(int handle, 
                        unsigned int size, 
                        const RECORDER_API4_RECORD * toc )
{
    DEBUGMSGTL(("rec:rec_log", "rec_descriptor_field\n"));
    return (_rec_api4(handle, size, REC_DESCRIBE_FIELD, toc));
}

int rec_api3(int handle, unsigned int code)
{
    rec_chif_resp resp;
    int rc;
    rec_chif_req req;
    REC_DETAIL_MSG *detail_msg;

    DEBUGMSGTL(("rec:rec_log", "rec_code_set\n"));
    memset (&req,0,sizeof(rec_chif_req));

    req.hdr.svc = RECORDER_CHIF_SERVICE;
    req.hdr.seq = ++s_sequence;

    req.msg.type = REC_BLACKBOX;

    req.msg.rec_msg.bb.subtype = REC_API3;

    detail_msg = &req.msg.rec_msg.bb.cmd.detail;
    detail_msg->handle = handle;
    //detail_msg->classs = cls;
    detail_msg->code = code;
    detail_msg->modify_mask = REC_MODIFY_CODE;

    if ((rc = doWriteRead(&req, sizeof(REC_DETAIL_MSG) + 16, &resp)) == 0){
        handle = resp.msg.rec_msg.returned.handle;
        DEBUGMSGTL(("rec:rec_log", "handle = %x\n", handle));
    }
    return (rc);
}

#ifdef notdef
int rec_build_filter(unsigned int size, 
                    const RECORDER_API4_RECORD * recs, 
                    UINT32 * f )
{
}
#endif

int rec_api5(int handle, 
              int type, 
              unsigned int bytes, 
              const UINT32 * f )
{
    rec_chif_resp resp;
    int rc;
    rec_chif_req req;
    REC_API5_MSG *filter_msg;

    DEBUGMSGTL(("rec:rec_log", "rec_filter\n"));
    memset (&req,0,sizeof(rec_chif_req));

    req.hdr.svc = RECORDER_CHIF_SERVICE;
    req.hdr.seq = ++s_sequence;

    req.msg.type = REC_BLACKBOX;

    req.msg.rec_msg.bb.subtype = REC_API5;

    filter_msg = &req.msg.rec_msg.bb.cmd.filter;
    filter_msg->handle = handle;
    filter_msg->bytes = bytes;
    if (bytes != 0) 
        memcpy(&filter_msg->filter,f,bytes*sizeof(UINT32));

    if ((rc = doWriteRead(&req, sizeof(REC_API5_MSG) + 16 + bytes, &resp)) == 0) {
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
    rec_chif_resp resp;
    int rc;
    rec_chif_req req;
    REC_LOG_MSG *log_msg;

    DEBUGMSGTL(("rec:rec_log", "_rec_log\n"));
    memset(&req,  0,sizeof(rec_chif_req));
    memset(&resp, 0, sizeof(rec_chif_resp));

    req.hdr.svc = RECORDER_CHIF_SERVICE;
    req.hdr.seq = ++s_sequence;

    req.msg.type = REC_BLACKBOX;

    req.msg.rec_msg.bb.subtype = log_type;

    log_msg = &req.msg.rec_msg.bb.cmd.log;

    log_msg->field = field;
    log_msg->instance = instance;
    log_msg->handle = handle;
    log_msg->size = size;
    if (size != 0) 
        memcpy(log_msg->data, data, size);

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
    DEBUGMSGTL(("rec:rec_log", "rec_log\n"));
    return (_rec_api6(handle, REC_API6, 0, 0, data, size));
}

int rec_api6_array(int handle, 
                 const char * data, 
                 unsigned int size )
{
    DEBUGMSGTL(("rec:rec_log", "rec_log_array\n"));
    return (_rec_api6(handle, REC_API7, 0, 0, data, size));
}

int rec_api6_instance(int handle, 
                    unsigned int instance, 
                    const char * data, 
                    unsigned int size )
{
    DEBUGMSGTL(("rec:rec_log", "rec_log_instance\n"));
    return (_rec_api6(handle, REC_API9, instance, 0, data, size));
}

int rec_api6_field(int handle, 
                    unsigned int instance, 
                    unsigned int field, 
                    const char * data, 
                    unsigned int size )
{
    DEBUGMSGTL(("rec:rec_log", "rec_log_field\n"));
    return (_rec_api6(handle, REC_API8, instance, field, data, size));
}

int rec_api6_static(int handle, 
                  const char * data, 
                  unsigned int size )
{
    DEBUGMSGTL(("rec:rec_log", "rec_log_static\n"));
    return (_rec_api6(handle, REC_API11, 0, 0, data, size));
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

