#include "../stdafx.h"

char *WSAErrorToString(int error)
{
	switch(error)
	{
		case WSATRY_AGAIN:
			return "WSATRY_AGAIN (11002) : This is usually a temporary error during hostname resolution and means that the local server did not receive a response from an authoritative server.";
		case WSANO_RECOVERY:
			return "WSANO_RECOVERY (11003) : A non-recoverable error occurred during a database lookup.";
		case WSANO_DATA:
			return "WSANO_DATA (11004) : The requested name is valid, but no data of the requested type was found.";
		case WSA_QOS_RECEIVERS:
			return "WSA_QOS_RECEIVERS (11005) : At least one reserve has arrived.";
		case WSA_QOS_SENDERS:
			return "WSA_QOS_SENDERS (11006) : At least one path has arrived.";
		case WSA_QOS_NO_SENDERS:
			return "WSA_QOS_NO_SENDERS (11007) : There are no senders.";
		case WSA_QOS_NO_RECEIVERS:
			return "WSA_QOS_NO_RECEIVERS (11008) : There are no receivers.";
		case WSA_QOS_REQUEST_CONFIRMED:
			return "WSA_QOS_REQUEST_CONFIRMED (11009) : Reserve has been confirmed.";
		case WSA_QOS_ADMISSION_FAILURE:
			return "WSA_QOS_ADMISSION_FAILURE (11010) : Error due to lack of resources.";
		case WSA_QOS_POLICY_FAILURE:
			return "WSA_QOS_POLICY_FAILURE (11011) : Rejected for administrative reasons - bad credentials.";
		case WSA_QOS_BAD_STYLE:
			return "WSA_QOS_BAD_STYLE (11012) : Unknown or conflicting style.";
		case WSA_QOS_BAD_OBJECT:
			return "WSA_QOS_BAD_OBJECT (11013) : Problem with some part of the filterspec or providerspecific buffer in general.";
		case WSA_QOS_TRAFFIC_CTRL_ERROR:
			return "WSA_QOS_TRAFFIC_CTRL_ERROR (11014) : Problem with some part of the flowspec.";
		case WSA_QOS_GENERIC_ERROR:
			return "WSA_QOS_GENERIC_ERROR (11015) : General QOS error.";
		case WSA_QOS_ESERVICETYPE:
			return "WSA_QOS_ESERVICETYPE (11016) : An invalid or unrecognized service type was found in the flowspec.";
		case WSA_QOS_EFLOWSPEC:
			return "WSA_QOS_EFLOWSPEC (11017) : An invalid or inconsistent flowspec was found in the QOS structure.";
		case WSA_QOS_EPROVSPECBUF:
			return "WSA_QOS_EPROVSPECBUF (11018) : Invalid QOS provider-specific buffer.";
		case WSA_QOS_EFILTERSTYLE:
			return "WSA_QOS_EFILTERSTYLE (11019) : An invalid QOS filter style was used.";
		case WSA_QOS_EFILTERTYPE:
			return "WSA_QOS_EFILTERTYPE (11020) : An invalid QOS filter type was used.";
		case WSA_QOS_EFILTERCOUNT:
			return "WSA_QOS_EFILTERCOUNT (11021) : An incorrect number of QOS FILTERSPECs were specified in the FLOWDESCRIPTOR.";
		case WSA_QOS_EOBJLENGTH:
			return "WSA_QOS_EOBJLENGTH (11022) : An object with an invalid ObjectLength field was specified in the QOS provider-specific buffer.";
		case WSA_QOS_EFLOWCOUNT:
			return "WSA_QOS_EFLOWCOUNT (11023) : An incorrect number of flow descriptors was specified in the QOS structure.";
		case WSA_QOS_EUNKOWNPSOBJ:
			return "WSA_QOS_EUNKOWNPSOBJ (11024) : An unrecognized object was found in the QOS provider-specific buffer.";
		case WSAHOST_NOT_FOUND:
			return "WSAHOST_NOT_FOUND (11001) : No such host is known.";
		case WSAEINTR:
			return "WSAEINTR (10004) : A blocking operation was interrupted by a call to WSACancelBlockingCall.";
		case WSAEBADF:
			return "WSAEBADF (10009) : The file handle supplied is not valid.";
		case WSAEACCES:
			return "WSAEACCES (10013) : An attempt was made to access a socket in a way forbidden by its access permissions.";
		case WSAEFAULT:
			return "WSAEFAULT (10014) : The system detected an invalid pointer address in attempting to use a pointer argument in a call.";
		case WSAEINVAL:
			return "WSAEINVAL (10022) : An invalid argument was supplied.";
		case WSAEMFILE:
			return "WSAEMFILE (10024) : Too many open sockets.";
		case WSAEWOULDBLOCK:
			return "WSAEWOULDBLOCK (10035) : A non-blocking socket operation could not be completed immediately.";
		case WSAEINPROGRESS:
			return "WSAEINPROGRESS (10036) : A blocking operation is currently executing.";
		case WSAEALREADY:
			return "WSAEALREADY (10037) : An operation was attempted on a non-blocking socket that already had an operation in progress.";
		case WSAENOTSOCK:
			return "WSAENOTSOCK (10038L) : An operation was attempted on something that is not a socket.";
		case WSAEDESTADDRREQ:
			return "WSAEDESTADDRREQ (10039) : A required address was omitted from an operation on a socket.";
		case WSAEMSGSIZE:
			return "WSAEMSGSIZE (10040) : A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram into was smaller than the datagram itself.";
		case WSAEPROTOTYPE:
			return "WSAEPROTOTYPE (10041) : A protocol was specified in the socket function call that does not support the semantics of the socket type requested.";
		case WSAENOPROTOOPT:
			return "WSAENOPROTOOPT (10042) : An unknown, invalid, or unsupported option or level was specified in a getsockopt or setsockopt call.";
		case WSAEPROTONOSUPPORT:
			return "WSAEPROTONOSUPPORT (10043) : The requested protocol has not been configured into the system, or no implementation for it exists.";
		case WSAESOCKTNOSUPPORT:
			return "WSAESOCKTNOSUPPORT (10044) : The support for the specified socket type does not exist in this address family.";
		case WSAEOPNOTSUPP:
			return "WSAEOPNOTSUPP (10045) : The attempted operation is not supported for the type of object referenced.";
		case WSAEPFNOSUPPORT:
			return "WSAEPFNOSUPPORT (10046) : The protocol family has not been configured into the system or no implementation for it exists.";
		case WSAEAFNOSUPPORT:
			return "WSAEAFNOSUPPORT (10047) : An address incompatible with the requested protocol was used.";
		case WSAEADDRINUSE:
			return "WSAEADDRINUSE (10048) : Only one usage of each socket address (protocol/network address/port) is normally permitted.";
		case WSAEADDRNOTAVAIL:
			return "WSAEADDRNOTAVAIL (10049) : The requested address is not valid in its context.";
		case WSAENETDOWN:
			return "WSAENETDOWN (10050) : A socket operation encountered a dead network.";
		case WSAENETUNREACH:
			return "WSAENETUNREACH (10051) : A socket operation was attempted to an unreachable network.";
		case WSAENETRESET:
			return "WSAENETRESET (10052) : The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress.";
		case WSAECONNABORTED:
			return "WSAECONNABORTED (10053) : An established connection was aborted by the software in your host machine.";
		case WSAECONNRESET:
			return "WSAECONNRESET (10054) : An existing connection was forcibly closed by the remote host.";
		case WSAENOBUFS:
			return "WSAENOBUFS (10055) : An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full.";
		case WSAEISCONN:
			return "WSAEISCONN (10056) : A connect request was made on an already connected socket.";
		case WSAENOTCONN:
			return "WSAENOTCONN (10057) : A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using a sendto call) no address was supplied.";
		case WSAESHUTDOWN:
			return "WSAESHUTDOWN (10058) : A request to send or receive data was disallowed because the socket had already been shut down in that direction with a previous shutdown call.";
		case WSAETOOMANYREFS:
			return "WSAETOOMANYREFS (10059) : Too many references to some kernel object.";
		case WSAETIMEDOUT:
			return "WSAETIMEDOUT (10060) : A connection attempt failed because the connected party did not properly respond after a period of time, or established connection failed because connected host has failed to respond.";
		case WSAECONNREFUSED:
			return "WSAECONNREFUSED (10061) : No connection could be made because the target machine actively refused it.";
		case WSAELOOP:
			return "WSAELOOP (10062) : Cannot translate name.";
		case WSAENAMETOOLONG:
			return "WSAENAMETOOLONG (10063) : Name component or name was too long.";
		case WSAEHOSTDOWN:
			return "WSAEHOSTDOWN (10064) : A socket operation failed because the destination host was down.";
		case WSAEHOSTUNREACH:
			return "WSAEHOSTUNREACH (10065) : A socket operation was attempted to an unreachable host.";
		case WSAENOTEMPTY:
			return "WSAENOTEMPTY (10066) : Cannot remove a directory that is not empty.";
		case WSAEPROCLIM:
			return "WSAEPROCLIM (10067) :	A Windows Sockets implementation may have a limit on the number of applications that may use it simultaneously.";
		case WSAEUSERS:
			return "WSAEUSERS (10068) : Ran out of quota.";
		case WSAEDQUOT:
			return "WSAEDQUOT (10069) : Ran out of disk quota.";
		case WSAESTALE:
			return "WSAESTALE (10070) : File handle reference is no longer available.";
		case WSAEREMOTE:
			return "WSAEREMOTE (10071) : Item is not available locally.";
		case WSASYSNOTREADY:
			return "WSASYSNOTREADY (10091) : WSAStartup cannot function at this time because the underlying system it uses to provide network services is currently unavailable.";
		case WSAVERNOTSUPPORTED:
			return "WSAVERNOTSUPPORTED (10092) : The Windows Sockets version requested is not supported.";
		case WSANOTINITIALISED:
			return "WSANOTINITIALISED (10093) : Either the application has not called WSAStartup, or WSAStartup failed.";
		case WSAEDISCON:
			return "WSAEDISCON (10101) : Returned by WSARecv or WSARecvFrom to indicate the remote party has initiated a graceful shutdown sequence.";
		case WSAENOMORE:
			return "WSAENOMORE (10102) : No more results can be returned by WSALookupServiceNext.";
		case WSAECANCELLED:
			return "WSAECANCELLED (10103) : A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.";
		case WSAEINVALIDPROCTABLE:
			return "WSAEINVALIDPROCTABLE (10104) : The procedure call table is invalid.";
		case WSAEINVALIDPROVIDER:
			return "WSAEINVALIDPROVIDER (10105) : The requested service provider is invalid.";
		case WSAEPROVIDERFAILEDINIT:
			return "WSAEPROVIDERFAILEDINIT (10106) : The requested service provider could not be loaded or initialized.";
		case WSASYSCALLFAILURE:
			return "WSASYSCALLFAILURE (10107) : A system call that should never fail has failed.";
		case WSASERVICE_NOT_FOUND:
			return "WSASERVICE_NOT_FOUND (10108) : No such service is known. The service cannot be found in the specified name space.";
		case WSATYPE_NOT_FOUND:
			return "WSATYPE_NOT_FOUND (10109) : The specified class was not found.";
		case WSA_E_NO_MORE:
			return "WSA_E_NO_MORE (10110) : No more results can be returned by WSALookupServiceNext.";
		case WSA_E_CANCELLED:
			return "WSA_E_CANCELLED (10111) : A call to WSALookupServiceEnd was made while this call was still processing. The call has been canceled.";
		case WSAEREFUSED:
			return "WSAEREFUSED (10112) : A database query failed because it was actively refused.";
	}

	return "Unknown code.";
}