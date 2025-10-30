/**************************************************************************************************************
 * altobeam RTOS
 *
 * Copyright (c) 2018, altobeam.inc   All rights reserved.
 *
 *  The source code contains proprietary information of AltoBeam, and shall not be distributed, 
 *  copied, reproduced, or disclosed in whole or in part without prior written permission of AltoBeam.
*****************************************************************************************************************/

#ifndef _ATBM_API_TYPE_H_
#define _ATBM_API_TYPE_H_

//#define jiffies 1000
#define ATBM_ALIGN(a,b)			(((a) + ((b) - 1)) & (~((b)-1)))
#define BITS_PER_BYTE		8
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define BITS_TO_LONGS(nr)	DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))

#define  ATBM_EPERM     1  /* Operation not permitted */
#define  ATBM_ENOENT     2  /* No such file or directory */
#define  ATBM_ESRCH     3  /* No such process */
#define  ATBM_EINTR     4  /* Interrupted system call */
#define  ATBM_EIO     5  /* I/O error */
#define  ATBM_ENXIO     6  /* No such device or address */
#define  ATBM_E2BIG     7  /* Arg list too long */
#define  ATBM_ENOEXEC     8  /* Exec format error */
#define  ATBM_EBADF     9  /* Bad file number */
#define  ATBM_ECHILD    10  /* No child processes */
#define  ATBM_EAGAIN    11  /* Try again */
#define  ATBM_ENOMEM    12  /* Out of memory */
#define  ATBM_EACCES    13  /* Permission denied */
#define  ATBM_EFAULT    14  /* Bad address */
#define  ATBM_ENOTBLK    15  /* Block device required */
#define  ATBM_EBUSY    16  /* Device or resource busy */
#define  ATBM_EEXIST    17  /* File exists */
#define  ATBM_EXDEV    18  /* Cross-device link */
#define  ATBM_ENODEV    19  /* No such device */
#define  ATBM_ENOTDIR    20  /* Not a directory */
#define  ATBM_EISDIR    21  /* Is a directory */
#define  ATBM_EINVAL    22  /* Invalid argument */
#define  ATBM_ENFILE    23  /* File table overflow */
#define  ATBM_EMFILE    24  /* Too many open files */
#define  ATBM_ENOTTY    25  /* Not a typewriter */
#define  ATBM_ETXTBSY    26  /* Text file busy */
#define  ATBM_EFBIG    27  /* File too large */
#define  ATBM_ENOSPC    28  /* No space left on device */
#define  ATBM_ESPIPE    29  /* Illegal seek */
#define  ATBM_EROFS    30  /* Read-only file system */
#define  ATBM_EMLINK    31  /* Too many links */
#define  ATBM_EPIPE    32  /* Broken pipe */
#define  ATBM_EDOM    33  /* Math argument out of domain of func */
#define  ATBM_ERANGE    34  /* Math result not representable */
#define  ATBM_EDEADLK    35  /* Resource deadlock would occur */
#define  ATBM_ENAMETOOLONG  36  /* File name too long */
#define  ATBM_ENOLCK    37  /* No record locks available */
#define  ATBM_ENOSYS    38  /* Function not implemented */
#define  ATBM_ENOTEMPTY  39  /* Directory not empty */
#define  ATBM_ELOOP    40  /* Too many symbolic links encountered */
#define  ATBM_EWOULDBLOCK  ATBM_EAGAIN  /* Operation would block */
#define  ATBM_ENOMSG    42  /* No message of desired type */
#define  ATBM_EIDRM    43  /* Identifier removed */
#define  ATBM_ECHRNG    44  /* Channel number out of range */
#define  ATBM_EL2NSYNC  45  /* Level 2 not synchronized */
#define  ATBM_EL3HLT    46  /* Level 3 halted */
#define  ATBM_EL3RST    47  /* Level 3 reset */
#define  ATBM_ELNRNG    48  /* Link number out of range */
#define  ATBM_EUNATCH    49  /* Protocol driver not attached */
#define  ATBM_ENOCSI    50  /* No CSI structure available */
#define  ATBM_EL2HLT    51  /* Level 2 halted */
#define  ATBM_EBADE    52  /* Invalid exchange */
#define  ATBM_EBADR    53  /* Invalid request descriptor */
#define  ATBM_EXFULL    54  /* Exchange full */
#define  ATBM_ENOANO    55  /* No anode */
#define  ATBM_EBADRQC    56  /* Invalid request code */
#define  ATBM_EBADSLT    57  /* Invalid slot */
#define  ATBM_EBFONT    59  /* Bad font file format */
#define  ATBM_ENOSTR    60  /* Device not a stream */
#define  ATBM_ENODATA    61  /* No data available */
#define  ATBM_ETIME    62  /* Timer expired */
#define  ATBM_ENOSR    63  /* Out of streams resources */
#define  ATBM_ENONET    64  /* Machine is not on the network */
#define  ATBM_ENOPKG    65  /* Package not installed */
#define  ATBM_EREMOTE    66  /* Object is remote */
#define  ATBM_ENOLINK    67  /* Link has been severed */
#define  ATBM_EADV    68  /* Advertise error */
#define  ATBM_ESRMNT    69  /* Srmount error */
#define  ATBM_ECOMM    70  /* Communication error on send */
#define  ATBM_EPROTO    71  /* Protocol error */
#define  ATBM_EMULTIHOP  72  /* Multihop attempted */
#define  ATBM_EDOTDOT    73  /* RFS specific error */
#define  ATBM_EBADMSG    74  /* Not a data message */
#define  ATBM_EOVERFLOW  75  /* Value too large for defined data type */
#define  ATBM_ENOTUNIQ  76  /* Name not unique on network */
#define  ATBM_EBADFD    77  /* File descriptor in bad state */
#define  ATBM_EREMCHG    78  /* Remote address changed */
#define  ATBM_ELIBACC    79  /* Can not access a needed shared library */
#define  ATBM_ELIBBAD    80  /* Accessing a corrupted shared library */
#define  ATBM_ELIBSCN    81  /* .lib section in a.out corrupted */
#define  ATBM_ELIBMAX    82  /* Attempting to link in too many shared libraries */
#define  ATBM_ELIBEXEC  83  /* Cannot exec a shared library directly */
#define  ATBM_EILSEQ    84  /* Illegal byte sequence */
#define  ATBM_ERESTART  85  /* Interrupted system call should be restarted */
#define  ATBM_ESTRPIPE  86  /* Streams pipe error */
#define  ATBM_EUSERS    87  /* Too many users */
#define  ATBM_ENOTSOCK  88  /* Socket operation on non-socket */
#define  ATBM_EDESTADDRREQ  89  /* Destination address required */
#define  ATBM_EMSGSIZE  90  /* Message too long */
#define  ATBM_EPROTOTYPE  91  /* Protocol wrong type for socket */
#define  ATBM_ENOPROTOOPT  92  /* Protocol not available */
#define  ATBM_EPROTONOSUPPORT  93  /* Protocol not supported */
#define  ATBM_ESOCKTNOSUPPORT  94  /* Socket type not supported */
#define  ATBM_EOPNOTSUPP  95  /* Operation not supported on transport endpoint */
#define  ATBM_EPFNOSUPPORT  96  /* Protocol family not supported */
#define  ATBM_EAFNOSUPPORT  97  /* Address family not supported by protocol */
#define  ATBM_EADDRINUSE  98  /* Address already in use */
#define  ATBM_EADDRNOTAVAIL  99  /* Cannot assign requested address */
#define  ATBM_ENETDOWN  100  /* Network is down */
#define  ATBM_ENETUNREACH  101  /* Network is unreachable */
#define  ATBM_ENETRESET  102  /* Network dropped connection because of reset */
#define  ATBM_ECONNABORTED  103  /* Software caused connection abort */
#define  ATBM_ECONNRESET  104  /* Connection reset by peer */
#define  ATBM_ENOBUFS    105  /* No buffer space available */
#define  ATBM_EISCONN    106  /* Transport endpoint is already connected */
#define  ATBM_ENOTCONN  107  /* Transport endpoint is not connected */
#define  ATBM_ESHUTDOWN  108  /* Cannot send after transport endpoint shutdown */
#define  ATBM_ETOOMANYREFS  109  /* Too many references: cannot splice */
#define  ATBM_ETIMEDOUT  110  /* Connection timed out */
#define  ATBM_ECONNREFUSED  111  /* Connection refused */
#define  ATBM_EHOSTDOWN  112  /* Host is down */
#define  ATBM_EHOSTUNREACH  113  /* No route to host */
#define  ATBM_EALREADY  114  /* Operation already in progress */
#define  ATBM_EINPROGRESS  115  /* Operation now in progress */
#define  ATBM_ESTALE    116  /* Stale NFS file handle */
#define  ATBM_EUCLEAN    117  /* Structure needs cleaning */
#define  ATBM_ENOTNAM    118  /* Not a XENIX named type file */
#define  ATBM_ENAVAIL    119  /* No XENIX semaphores available */
#define  ATBM_EISNAM    120  /* Is a named type file */
#define  ATBM_EREMOTEIO  121  /* Remote I/O error */
#define  ATBM_EDQUOT    122  /* Quota exceeded */

#define  ATBM_ENOMEDIUM  123  /* No medium found */
#define  ATBM_EMEDIUMTYPE  124  /* Wrong medium type */


#define ATBM_ENSROK    0 /* DNS server returned answer with no data */
#define ATBM_ENSRNODATA  160 /* DNS server returned answer with no data */
#define ATBM_ENSRFORMERR 161 /* DNS server claims query was misformatted */
#define ATBM_ENSRSERVFAIL 162  /* DNS server returned general failure */
#define ATBM_ENSRNOTFOUND 163  /* Domain name not found */
#define ATBM_ENSRNOTIMP  164 /* DNS server does not implement requested operation */
#define ATBM_ENSRREFUSED 165 /* DNS server refused query */
#define ATBM_ENSRBADQUERY 166  /* Misformatted DNS query */
#define ATBM_ENSRBADNAME 167 /* Misformatted domain name */
#define ATBM_ENSRBADFAMILY 168 /* Unsupported address family */
#define ATBM_ENSRBADRESP 169 /* Misformatted DNS reply */
#define ATBM_ENSRCONNREFUSED 170 /* Could not contact DNS servers */
#define ATBM_ENSRTIMEOUT 171 /* Timeout while contacting DNS servers */
#define ATBM_ENSROF    172 /* End of file */
#define ATBM_ENSRFILE  173 /* Error reading file */
#define ATBM_ENSRNOMEM 174 /* Out of memory */
#define ATBM_ENSRDESTRUCTION 175 /* Application terminated lookup */
#define ATBM_ENSRQUERYDOMAINTOOLONG  176 /* Domain name is too long */
#define ATBM_ENSRCNAMELOOP 177 /* Domain name is too long */
#define ATBM_ENOTSUPP      178 /*no support */


/*
;*****************************************************************************************************
;* 描	 述 : 定义系统的数据类型。
;*****************************************************************************************************
;*/
typedef unsigned char ATBM_BOOL;
typedef unsigned char atbm_uint8;		/* Unsigned  8 bit quantity							*/
typedef signed char atbm_int8;			/* Signed    8 bit quantity							 */
typedef unsigned short atbm_uint16; 	/* Unsigned 16 bit quantity							  */
typedef signed short atbm_int16;		/* Signed	16 bit quantity 						   */
typedef unsigned int atbm_uint32;		/* Unsigned 32 bit quantity							*/
typedef signed int atbm_int32;			/* Signed   32 bit quantity							 */
typedef float atbm_fp32;				/* Single precision floating point					   */
typedef unsigned long long atbm_uint64;	/* Unsigned  64 bit quantity							*/
typedef long long atbm_int64;			/* Signed    64 bit quantity							 */
typedef void atbm_void;
typedef atbm_uint32   ATBM_OS_CPU_SR;		/* Define size of CPU status register (PSR = 32 bits) */
typedef atbm_int32  atbm_size_t;
typedef unsigned long atbm_uint32_lock;
typedef long os_time_t;

struct os_time {
	os_time_t sec;
	os_time_t usec;
};

/*ATBM_FALSE & ATBM_TRUE*/
#define ATBM_FALSE	0
#define ATBM_TRUE	1

/*Return value defination*/
#define WIFI_OK				0 	
#define WIFI_ERROR			-1
#ifndef LINUX_OS
#ifndef BIT
#define BIT(x)	(1 << (x))
#endif
#endif
#define NETDEV_ALIGN		32
#define ETHTOOL_BUSINFO_LEN 32


#define atbm_min(_a, _b) 	(((_a) < (_b)) ? (_a) : (_b))
#define atbm_max(_a, _b) 	(((_a) > (_b)) ? (_a) : (_b))


#define ATBM_NULL	((atbm_void *)0)

#define atbm_likely(a) (a)
#define atbm_unlikely(a) (a)

#define atbm_cpu_to_le16(v16) (v16)
#define atbm_cpu_to_le32(v32) (v32)
#define atbm_cpu_to_le64(v64) (v64)
#define atbm_le16_to_cpu(v16) (v16)
#define atbm_le32_to_cpu(v32) (v32)
#define atbm_le64_to_cpu(v64) (v64)

#define __atbm_cpu_to_le16(v16) (v16)
#define __atbm_cpu_to_le64(v64) (v64)
#define __atbm_le16_to_cpu(v16) (v16)
#define __atbm_le32_to_cpu(v32) (v32)

#define atbm_cpu_to_be16(a) atbm_htons(a)
#define atbm_cpu_to_be32(a) atbm_htonl(a)
#define atbm_be16_to_cpu(a) atbm_ntohs(a)
#define atbm_be32_to_cpu(a) atbm_ntohl(a)

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#ifndef GENMASK
#define BITS_PER_LONG 32
#define GENMASK(h, l) \
	(((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
#endif

static inline atbm_uint32 atbm_le32_get_bits(atbm_uint32 v, atbm_uint32 field)
{
	return (atbm_le32_to_cpu(v) & field)/(field & -field);
}

#endif /*_API_TYPE_H_*/
