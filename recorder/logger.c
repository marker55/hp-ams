#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "recorder.h"
#include "net-snmp/net-snmp-config.h"
#include "net-snmp/library/snmp_impl.h"
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/output_api.h>
#include "net-snmp/library/tools.h"

#ifndef true
#define true 1
#endif
#ifndef false
#define false 1
#endif

#ifdef UTEST
void dump_chif(ilo_req * req) {
    fprintf(stderr, "pktsz = %d ",req->hdr.pktsz);
    fprintf(stderr, "seq = %d   ",req->hdr.seq);
    fprintf(stderr, "cmd = %d   ",req->hdr.cmd);
    fprintf(stderr, "svc = %d\n",req->hdr.svc);
}

void dump_alloc_req(ilo_req* req) {
    fprintf(stderr, "CHIF Request: ");
    dump_chif(req);

    fprintf(stderr, "request->type = %d\n",req->msg.type);
    fprintf(stderr, "request->rec.subtype = %d\n",
				req->msg.rec.chunk.subtype);
    fprintf(stderr, "request->rec.cmd.alloc.name = %s\n",
				req->msg.rec.chunk.cmd.alloc.name);
}

void dump_alloc_resp(ilo_resp* resp) {
    fprintf(stderr, "CHIF Response: ");
    dump_chif(resp);

    fprintf(stderr, "response->rcode = %d\n",resp->msg.rcode);
    fprintf(stderr, "response->rec.returned.handle = %d\n",
				resp->msg.rec.returned.handle);
}

#else
void dump_chif(ilo_req* req) {
}
void dump_alloc_req(ilo_req* resp) {
}
void dump_alloc_resp(ilo_resp* resp) {
}

#endif
int doWrite( ilo_req *, int);
int doRead( void *, int);
int doWriteRead(void *, int, ilo_resp *);

int s_class = -1;
int s_handle = -1;

static uint16_t s_sequence = 0;

/* Only available for OS clients (via CHIF) for OS API callers from _service */
int recorder0( const char* name )
{
    ilo_req req;
    ilo_resp resp;
    int  size;
    RECORDER0_MSG *recorder0_msg; 

    DEBUGMSGTL(("recorder:log", "recorder0\n"));
    memset (&req,0,sizeof(ilo_req));
    
    req.hdr.svc = 0x11;
    req.hdr.seq = ++s_sequence;

    req.msg.type = 2;
    req.msg.rec.chunk.subtype = 13;
    recorder0_msg = &req.msg.rec.chunk.cmd.recorder0;
    strncpy(recorder0_msg->name, name, sizeof(recorder0_msg->name) - 1);
    recorder0_msg->name[sizeof(recorder0_msg->name) - 1] = 0;
	//size = (int)alloc_msg - (int)&req + sizeof(alloc_msg->name) + 4;
	size = sizeof(pkt_hdr) + sizeof(RECORDER0_MSG) + 8;
	dump_alloc_req(&req);
    if (doWrite(&req, size) > 0 )
        if (doRead(&resp,sizeof(ilo_resp)) > 0) {
	    dump_alloc_resp(&resp);
            return (resp.msg.rcode);
        }
    
    return (-1);
}

int recorder1(const char * name, 
                unsigned int cls, 
                unsigned int flags, 
                unsigned int size, 
                int * handle )
{
    ilo_resp resp;
    int rc;
    ilo_req req;
    RECORDER1_MSG *recorder1_msg;

    DEBUGMSGTL(("recorder:log", "recorder1\n"));
    memset (&req, 0, sizeof(ilo_req));

    req.hdr.svc = 0x11;
    req.hdr.seq = ++s_sequence;

    req.msg.type = 2;

    req.msg.rec.chunk.subtype = 1;

    recorder1_msg = &req.msg.rec.chunk.cmd.recorder1;
    recorder1_msg->classs = cls;
    recorder1_msg->flags = flags;
    recorder1_msg->size = size;
    recorder1_msg->count = 0;

    strncpy(recorder1_msg->name, name, sizeof(recorder1_msg->name) - 1);
    recorder1_msg->name[sizeof(recorder1_msg->name) - 1] = 0;

    if ((rc =doWriteRead(&req, sizeof(ilo_req),&resp)) == 0)
        *handle = resp.msg.rec.returned.handle;
    return (rc);
}

int recorder1_array(const char * name, 
                      unsigned int cls, 
                      unsigned int flags, 
                      unsigned int size, 
                      unsigned int count, 
                      int * handle )
{
    ilo_resp resp;
    int rc;
    ilo_req req;
    RECORDER1_MSG *recorder1_msg;

    DEBUGMSGTL(("recorder:log", "recorder1_array\n"));
    memset (&req,0,sizeof(ilo_req));

    req.hdr.svc = 0x11;
    req.hdr.seq = ++s_sequence;

    req.msg.type = 2;

    req.msg.rec.chunk.subtype = 1;

    recorder1_msg = &req.msg.rec.chunk.cmd.recorder1;
    recorder1_msg->classs = cls;
    recorder1_msg->flags = flags;
    recorder1_msg->size = size;
    recorder1_msg->count = count;

    strncpy(recorder1_msg->name, name, sizeof(recorder1_msg->name) - 1);
    recorder1_msg->name[sizeof(recorder1_msg->name) - 1] = 0;

    if ((rc =doWriteRead(&req, sizeof(ilo_req),&resp)) == 0)
        *handle = resp.msg.rec.returned.handle;
    return (rc);
}

int recorder2(int handle)
{
    ilo_resp resp;
    int rc;
    ilo_req req;
    RECORDER2_MSG *recorder2_msg;

    DEBUGMSGTL(("recorder:log", "recorder2\n"));
    memset (&req,0,sizeof(ilo_req));

    req.hdr.svc = 0x11;
    req.hdr.seq = ++s_sequence;

    req.msg.type = 2;

    req.msg.rec.chunk.subtype = 2;

    recorder2_msg = &req.msg.rec.chunk.cmd.recorder2;
    recorder2_msg->handle = handle;

    if ((rc =doWriteRead(&req, sizeof(ilo_req),&resp)) == 0) {
        handle = resp.msg.rec.returned.handle;
        DEBUGMSGTL(("recorder:log", "handle = %x\n", handle));
    }
    return (rc);
}

static int _recorder4(int handle, 
                        unsigned int size, 
                        int type,
                        const DESCRIPTOR_RECORD * toc )
{
    ilo_resp resp;
    int rc;
    ilo_req req;
    RECORDER4_MSG *recorder4_msg;

    DEBUGMSGTL(("recorder:log", "_recorder4\n"));
    memset (&req,0,sizeof(ilo_req));

    req.hdr.svc = 0x11;
    req.hdr.seq = ++s_sequence;

    req.msg.type = 2;

    req.msg.rec.chunk.subtype = 4;

    recorder4_msg = &req.msg.rec.chunk.cmd.recorder4;
    recorder4_msg->handle = handle;
    recorder4_msg->size = size;
    recorder4_msg->selection = type;
    memcpy(&recorder4_msg->toc, toc, size);
    dump_chunk("recorder:log","Request Packet", (const u_char *)&req, 
                                             16+size + (3 * sizeof(int)));

    if ((rc =doWriteRead(&req, 16 + size + (3 * sizeof(int)) ,&resp)) == 0) {
        handle = resp.msg.rec.returned.handle;
        DEBUGMSGTL(("recorder:log", "handle = %x\n", handle));
    }
    return (rc);
}

int recorder4_class(int handle, 
                        unsigned int size, 
                        const DESCRIPTOR_RECORD * toc )
{
    DEBUGMSGTL(("recorder:log", "recorder4_class\n"));
    return (_recorder4(handle, size, DESCRIBE_CLASS, toc));
}

int recorder4_code(int handle, 
                        unsigned int size, 
                        const DESCRIPTOR_RECORD * toc )
{
    DEBUGMSGTL(("recorder:log", "recorder4_code\n"));
    return (_recorder4(handle, size, DESCRIBE_CODE, toc));
}

int recorder4_field(int handle, 
                        unsigned int size, 
                        const DESCRIPTOR_RECORD * toc )
{
    DEBUGMSGTL(("recorder:log", "recorder4_field\n"));
    return (_recorder4(handle, size, DESCRIBE_FIELD, toc));
}

int recorder3(int handle, unsigned int code)
{
    ilo_resp resp;
    int rc;
    ilo_req req;
    RECORDER3_MSG *recorder3_msg;

    DEBUGMSGTL(("recorder:log", "recorder3\n"));
    memset (&req,0,sizeof(ilo_req));

    req.hdr.svc = 0x11;
    req.hdr.seq = ++s_sequence;

    req.msg.type = 2;

    req.msg.rec.chunk.subtype = 3;

    recorder3_msg = &req.msg.rec.chunk.cmd.recorder3;
    recorder3_msg->handle = handle;
    recorder3_msg->code = code;
    recorder3_msg->modify_mask = MODIFY_CODE;

    if ((rc =doWriteRead(&req, sizeof(ilo_req),&resp)) == 0){
        handle = resp.msg.rec.returned.handle;
        DEBUGMSGTL(("recorder:log", "handle = %x\n", handle));
    }
    return (rc);
}

int recorder5(int handle, 
              int type, 
              unsigned int bytes, 
              const UINT32 * f )
{
    ilo_resp resp;
    int rc;
    ilo_req req;
    RECORDER5_MSG *recorder5_msg;

    DEBUGMSGTL(("recorder:log", "recorder5\n"));
    memset (&req,0,sizeof(ilo_req));

    req.hdr.svc = 0x11;
    req.hdr.seq = ++s_sequence;

    req.msg.type = 2;

    req.msg.rec.chunk.subtype = 5;
    recorder5_msg = &req.msg.rec.chunk.cmd.recorder5;
    recorder5_msg->handle = handle;
    recorder5_msg->bytes = bytes;
    if (bytes != 0) 
        memcpy(&recorder5_msg->recorder5,f,bytes*sizeof(UINT32));

    if ((rc =doWriteRead(&req, sizeof(ilo_req),&resp)) == 0) {
        handle = resp.msg.rec.returned.handle;
        DEBUGMSGTL(("recorder:log", "handle = %x\n", handle));
    }
    return (rc);
}

static int _record( int handle, 
            int type,
            int instance,
            int field,
            const char * data, 
            unsigned int size )
{
    ilo_resp resp;
    int rc;
    ilo_req req;
    RECORDER_MSG *recorder_msg;

    DEBUGMSGTL(("recorder:log", "_recorder\n"));
    memset (&req,0,sizeof(ilo_req));

    req.hdr.svc = 0x11;
    req.hdr.seq = ++s_sequence;

    req.msg.type = 2;

    req.msg.rec.chunk.subtype = type;

    recorder_msg = &req.msg.rec.chunk.cmd.recorder;

    recorder_msg->field = field;
    recorder_msg->instance = instance;
    recorder_msg->handle = handle;
    recorder_msg->size = size;
    if (size != 0) 
        memcpy(recorder_msg->data,data,size);

    if ((rc =doWriteRead(&req, sizeof(ilo_req),&resp)) == 0) {
        handle = resp.msg.rec.returned.handle;
        DEBUGMSGTL(("recorder:log", "handle = %x\n", handle));
    }
    return (rc);
}

int record( int handle, 
            const char * data, 
            unsigned int size )
{
    DEBUGMSGTL(("recorder:log", "record\n"));
    return (_record(handle, 6, 0, 0, data, size));
}

int record_array(int handle, 
                 const char * data, 
                 unsigned int size )
{
    DEBUGMSGTL(("recorder:log", "record_array\n"));
    return (_record(handle, 7, 0, 0, data, size));
}

int record_instance(int handle, 
                    unsigned int instance, 
                    const char * data, 
                    unsigned int size )
{
    DEBUGMSGTL(("recorder:log", "record_instance\n"));
    return (_record(handle, 9, instance, 0, data, size));
}

int record_field(int handle, 
                    unsigned int instance, 
                    unsigned int field, 
                    const char * data, 
                    unsigned int size )
{
    DEBUGMSGTL(("recorder:log", "record_field\n"));
    return (_record(handle, 8, instance, field, data, size));
}

int record_static(int handle, 
                  const char * data, 
                  unsigned int size )
{
    DEBUGMSGTL(("recorder:log", "record_static\n"));
    return (_record(handle, 11, 0, 0, data, size));
}

int recorder_init() {
    int rc;
    if (s_class == -1 ) { 
        if ((s_class = recorder0("AMS")) == -1) {
            DEBUGMSGTL(("recorder:log", "recorder0 failed\n"));
            return false;
        }
    }
    DEBUGMSGTL(("recorder:log", "AMS = %d\n",s_class));

    if (s_handle == -1 ) {
        if ((rc = recorder1("AMS", s_class, 
                  STATUS_NO_COMPARE, MAX_BLOB_SIZE, 
                  &s_handle)) != OK ) {
            DEBUGMSGTL(("recorder:log", "recorder1 failed (%d)\n", 
                            rc));
            return false;
        }
    }
    DEBUGMSGTL(("recorder:log", "Handle = %x\n", s_handle));
    return true;
}

