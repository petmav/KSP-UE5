using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.Remoting.Activation;
using System.Runtime.Serialization;
using System.Security;

namespace System.Runtime.Remoting.Messaging;

[Serializable]
[ComVisible(true)]
[CLSCompliant(false)]
public class ConstructionResponse : MethodResponse, IConstructionReturnMessage, IMethodReturnMessage, IMethodMessage, IMessage
{
	public override IDictionary Properties
	{
		[SecurityCritical]
		get
		{
			return base.Properties;
		}
	}

	public ConstructionResponse(Header[] h, IMethodCallMessage mcm)
		: base(h, mcm)
	{
	}

	internal ConstructionResponse(object resultObject, LogicalCallContext callCtx, IMethodCallMessage msg)
		: base(resultObject, null, callCtx, msg)
	{
	}

	internal ConstructionResponse(Exception e, IMethodCallMessage msg)
		: base(e, msg)
	{
	}

	internal ConstructionResponse(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}
