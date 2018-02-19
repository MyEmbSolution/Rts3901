#include "global_ns.h"
#include "soapH.h"
#include "rsOnvifTypes.h"
#include "rsOnvifMsg.h"

SOAP_FMAC5 int SOAP_FMAC6 __tdn__Hello(struct soap *soap,
	struct wsdd__HelloType tdn__Hello,
	struct wsdd__ResolveType *tdn__HelloResponces)
{
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tdn__Bye(struct soap *soap,
	struct wsdd__ByeType tdn__Bye,
	struct wsdd__ResolveType *tdn__ByeResponces)
{
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tdn__Probe(struct soap *soap,
	struct wsdd__ProbeType tdn__Probe,
	struct wsdd__ProbeMatchesType *tdn__ProbeResponces)
{
	return 0;
}

//We will delete 3 following functions when we release our service finally.
SOAP_FMAC5 int SOAP_FMAC6 __tev__Notify_(struct soap *soap, struct _wsnt__Notify *wsnt__Notify)
{
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__Renew_(struct soap *soap,
	struct _wsnt__Renew *wsnt__Renew,
	struct _wsnt__RenewResponse *wsnt__RenewResponces)
{
	return 0;
}

SOAP_FMAC5 int SOAP_FMAC6 __tev__Unsubscribe_(struct soap *soap,
	struct _wsnt__Unsubscribe *wsnt__Unsubscribe,
	struct _wsnt__UnsubscribeResponse *wsnt__UnsubscribeResponces)
{
	return 0;
}