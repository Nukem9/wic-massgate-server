#include "../stdafx.h"

char *WSAErrorToString(int error)
{
	switch(error)
	{
		case 11002:
			return "WSATRY_AGAIN (11002) : This is usually a temporary error during hostname resolution and means that the local server did not receive a response from an authoritative server.";
		break;
		case 11003:
			return "WSANO_RECOVERY (11003) : A non-recoverable error occurred during a database lookup.";
		break;
		case 11004:
			return "WSANO_DATA (11004) : The requested name is valid, but no data of the requested type was found.";
		break;
		case 11005:
			return "WSA_QOS_RECEIVERS (11005) : At least one reserve has arrived.";
		break;
		case 11006:
			return "WSA_QOS_SENDERS (11006) : At least one path has arrived.";
		break;
		case 11007:
			return "WSA_QOS_NO_SENDERS (11007) : There are no senders.";
		break;
		case 11008:
			return "WSA_QOS_NO_RECEIVERS (11008) : There are no receivers.";
		break;
		case 11009:
			return "WSA_QOS_REQUEST_CONFIRMED (11009) : Reserve has been confirmed.";
		break;
		case 11010:
			return "WSA_QOS_ADMISSION_FAILURE (11010) : Error due to lack of resources.";
		break;
		case 11011:
			return "WSA_QOS_POLICY_FAILURE (11011) : Rejected for administrative reasons - bad credentials.";
		break;
		case 11012:
			return "WSA_QOS_BAD_STYLE (11012) : Unknown or conflicting style.";
		break;
		case 11013:
			return "WSA_QOS_BAD_OBJECT (11013) : Problem with some part of the filterspec or providerspecific buffer in general.";
		break;
		case 11014:
			return "WSA_QOS_TRAFFIC_CTRL_ERROR (11014) : Problem with some part of the flowspec.";
		break;
		case 11015:
			return "WSA_QOS_GENERIC_ERROR (11015) : General QOS error.";
		break;
		case 11016:
			return "WSA_QOS_ESERVICETYPE (11016) : An invalid or unrecognized service type was found in the flowspec.";
		break;
		case 11017:
			return "WSA_QOS_EFLOWSPEC (11017) : An invalid or inconsistent flowspec was found in the QOS structure.";
		break;
		case 11018:
			return "WSA_QOS_EPROVSPECBUF (11018) : Invalid QOS provider-specific buffer.";
		break;
		case 11019:
			return "WSA_QOS_EFILTERSTYLE (11019) : An invalid QOS filter style was used.";
		break;
		case 11020:
			return "WSA_QOS_EFILTERTYPE (11020) : An invalid QOS filter type was used.";
		break;
		case 11021:
			return "WSA_QOS_EFILTERCOUNT (11021) : An incorrect number of QOS FILTERSPECs were specified in the FLOWDESCRIPTOR.";
		break;
		case 11022:
			return "WSA_QOS_EOBJLENGTH (11022) : An object with an invalid ObjectLength field was specified in the QOS provider-specific buffer.";
		break;
		case 11023:
			return "WSA_QOS_EFLOWCOUNT (11023) : An incorrect number of flow descriptors was specified in the QOS structure.";
		break;
		case 11024:
			return "WSA_QOS_EUNKOWNPSOBJ (11024) : An unrecognized object was found in the QOS provider-specific buffer.";
		break;
		case 11001:
			return "WSAHOST_NOT_FOUND (11001) : No such host is known.";
		break;
		case 10004:
			return "WSAEINTR (10004) : A blocking operation was interrupted by a call to WSACancelBlockingCall.";
		break;
		case 10009:
			return "WSAEBADF (10009) : The file handle supplied is not valid.";
		break;
		case 10013:
			return "WSAEACCES (10013) : An attempt was made to access a socket in a way forbidden by its access permissions.";
		break;
		case 10014:
			return "WSAEFAULT (10014) : The system detected an invalid pointer address in attempting to use a pointer argument in a call.";
		break;
		case 10022:
			return "WSAEINVAL (10022) : An invalid argument was supplied.";
		break;
		case 10024:
			return "WSAEMFILE (10024) : Too many open sockets.";
		break;
		case 10035:
			return "WSAEWOULDBLOCK (10035) : A non-blocking socket operation could not be completed immediately.";
		break;
		case 10036:
			return "WSAEINPROGRESS (10036) : A blocking operation is currently executing.";
		break;
		case 10037:
			return "WSAEALREADY (10037) : An operation was attempted on a non-blocking socket that already had an operation in progress.";
		break;
		case 10038:
			return "WSAENOTSOCK (10038L) : An operation was attempted on something that is not a socket.";
		break;
		case 10039:
			return "WSAEDESTADDRREQ (10039) : A required address was omitted from an operation on a socket.";
		break;
		case 10040:
			return "WSAEMSGSIZE (10040) : A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram into was smaller than the datagram itself.";
		break;
		case 10041:
			return "WSAEPROTOTYPE (10041) : A protocol was specified in the socket function call that does not support the semantics of the socket type requested.";
		break;
		case 10042:
			return "WSAENOPROTOOPT (10042) : An unknown, invalid, or unsupported option or level was specified in a getsockopt or setsockopt call.";
		break;
		case 10043:
			return "WSAEPROTONOSUPPORT (10043) : The requested protocol has not been configured into the system, or no implementation for it exists.";
		break;
		case 10044:
			return "WSAESOCKTNOSUPPORT (10044) : The support for the specified socket type does not exist in this address family.";
		break;
		case 10045:
			return "WSAEOPNOTSUPP (10045) : The attempted operation is not supported for the type of object referenced.";
		break;
		case 10046:
			return "WSAEPFNOSUPPORT (10046) : The protocol family has not been configured into the system or no implementation for it exists.";
		break;
		case 10047:
			return "WSAEAFNOSUPPORT (10047) : An address incompatible with the requested protocol was used.";
		break;
		case 10048:
			return "WSAEADDRINUSE (10048) : Only one usage of each socket address (protocol/network address/port) is normally permitted.";
		break;
		case 10049:
			return "WSAEADDRNOTAVAIL (10049) : The requested address is not valid in its context.";
		break;
		case 10050:
			return "WSAENETDOWN (10050) : A socket operation encountered a dead network.";
		break;
		case 10051:
			return "WSAENETUNREACH (10051) : A socket operation was attempted to an unreachable network.";
		break;
		case 10052:
			return "WSAENETRESET (10052) : The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress.";
		break;
		case 10053:
			return "WSAECONNABORTED (10053) : An established connection was aborted by the software in your host machine.";
		break;
		case 10054:
			return "WSAECONNRESET (10054) : An existing connection was forcibly closed by the remote host.";
		break;
		case 10055:
			return "WSAENOBUFS (10055) : An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full.";
		break;
		case 10056:
			return "WSAEISCONN (10056) : A connect request was made on an already connected socket.";
		break;
		case 10057:
			return "WSAENOTCONN (10057) : A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using a sendto call) no address was supplied.";
		break;
		case 10058:
			return "WSAESHUTDOWN (10058) : A request to send or receive data was disallowed because the socket had already been shut down in that direction with a previous shutdown call.";
		break;
		case 10059:
			return "WSAETOOMANYREFS (10059) : Too many references to some kernel object.";
		break;
		case 10060:
			return "WSAETIMEDOUT (10060) : A connection attempt failed because the connected party did not properly respond after a period of time, or established connection failed because connected host has failed to respond.";
		break;
		case 10061:
			return "WSAECONNREFUSED (10061) : No connection could be made because the target machine actively refused it.";
		break;
		case 10062:
			return "WSAELOOP (10062) : Cannot translate name.";
		break;
		case 10063:
			return "WSAENAMETOOLONG (10063) : Name component or name was too long.";
		break;
		case 10064:
			return "WSAEHOSTDOWN (10064) : A socket operation failed because the destination host was down.";
		break;
		case 10065:
			return "WSAEHOSTUNREACH (10065) : A socket operation was attempted to an unreachable host.";
		break;
		case 10066:
			return "WSAENOTEMPTY (10066) : Cannot remove a directory that is not empty.";
		break;
		case 10067:
			return "WSAEPROCLIM (10067) :	A Windows Sockets implementation may have a limit on the number of applications that may use it simultaneously.";
		break;
		case 10068:
			return "WSAEUSERS (10068) : Ran out of quota.";
		break;
		case 10069:
			return "WSAEDQUOT (10069) : Ran out of disk quota.";
		break;
		case 10070:
			return "WSAESTALE (10070) : File handle reference is no longer available.";
		break;
		case 10071:
			return "WSAEREMOTE (10071) : Item is not available locally.";
		break;
		case 10091:
			return "WSASYSNOTREADY (10091) : WSAStartup cannot function at this time because the underlying system it uses to provide network services is currently unavailable.";
		break;
		case 10092:
			return "WSAVERNOTSUPPORTED (10092) : The Windows Sockets version requested is not supported.";
		break;
		case 10093:
			return "WSANOTINITIALISED (10093) : Either the application has not called WSAStartup, or WSAStartup failed.";
		break;
		case 10101:
			return "WSAEDISCON (10101) : Returned by WSARecv or WSARecvFrom to indicate the remote party has initiated a graceful shutdown sequence.";
		break;
		case 10102:
			return "WSAENOMORE (10102) : No more results can be returned by WSALookupServiceNext.";
		break;
		case 10103:
			return "WSAECANCELLED (10103) : A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.";
		break;
		case 10104:
			return "WSAEINVALIDPROCTABLE (10104) : The procedure call table is invalid.";
		break;
		case 10105:
			return "WSAEINVALIDPROVIDER (10105) : The requested service provider is invalid.";
		break;
		case 10106:
			return "WSAEPROVIDERFAILEDINIT (10106) : The requested service provider could not be loaded or initialized.";
		break;
		case 10107:
			return "WSASYSCALLFAILURE (10107) : A system call that should never fail has failed.";
		break;
		case 10108:
			return "WSASERVICE_NOT_FOUND (10108) : No such service is known. The service cannot be found in the specified name space.";
		break;
		case 10109:
			return "WSATYPE_NOT_FOUND (10109) : The specified class was not found.";
		break;
		case 10110:
			return "WSA_E_NO_MORE (10110) : No more results can be returned by WSALookupServiceNext.";
		break;
		case 10111:
			return "WSA_E_CANCELLED (10111) : A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.";
		break;
		case 10112:
			return "WSAEREFUSED (10112) : A database query failed because it was actively refused.";
		break;
	}

	return "Unknown code.";
}