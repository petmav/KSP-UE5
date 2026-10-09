using System.Collections;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security;

namespace System.Runtime.Remoting.Messaging;

[ComVisible(true)]
public class MethodCallMessageWrapper : InternalMessageWrapper, IMethodCallMessage, IMethodMessage, IMessage
{
	private class DictionaryWrapper : MCMDictionary
	{
		private IDictionary _wrappedDictionary;

		private static string[] _keys = new string[1] { "__Args" };

		public DictionaryWrapper(IMethodMessage message, IDictionary wrappedDictionary)
			: base(message)
		{
			_wrappedDictionary = wrappedDictionary;
			base.MethodKeys = _keys;
		}

		protected override IDictionary AllocInternalProperties()
		{
			return _wrappedDictionary;
		}

		protected override void SetMethodProperty(string key, object value)
		{
			if (key == "__Args")
			{
				((MethodCallMessageWrapper)_message)._args = (object[])value;
			}
			else
			{
				base.SetMethodProperty(key, value);
			}
		}

		protected override object GetMethodProperty(string key)
		{
			if (key == "__Args")
			{
				return ((MethodCallMessageWrapper)_message)._args;
			}
			return base.GetMethodProperty(key);
		}
	}

	private object[] _args;

	private ArgInfo _inArgInfo;

	private DictionaryWrapper _properties;

	public virtual int ArgCount
	{
		[SecurityCritical]
		get
		{
			return ((IMethodCallMessage)WrappedMessage).ArgCount;
		}
	}

	public virtual object[] Args
	{
		[SecurityCritical]
		get
		{
			return _args;
		}
		set
		{
			_args = value;
		}
	}

	public virtual bool HasVarArgs
	{
		[SecurityCritical]
		get
		{
			return ((IMethodCallMessage)WrappedMessage).HasVarArgs;
		}
	}

	public virtual int InArgCount
	{
		[SecurityCritical]
		get
		{
			return _inArgInfo.GetInOutArgCount();
		}
	}

	public virtual object[] InArgs
	{
		[SecurityCritical]
		get
		{
			return _inArgInfo.GetInOutArgs(_args);
		}
	}

	public virtual LogicalCallContext LogicalCallContext
	{
		[SecurityCritical]
		get
		{
			return ((IMethodCallMessage)WrappedMessage).LogicalCallContext;
		}
	}

	public virtual MethodBase MethodBase
	{
		[SecurityCritical]
		get
		{
			return ((IMethodCallMessage)WrappedMessage).MethodBase;
		}
	}

	public virtual string MethodName
	{
		[SecurityCritical]
		get
		{
			return ((IMethodCallMessage)WrappedMessage).MethodName;
		}
	}

	public virtual object MethodSignature
	{
		[SecurityCritical]
		get
		{
			return ((IMethodCallMessage)WrappedMessage).MethodSignature;
		}
	}

	public virtual IDictionary Properties
	{
		[SecurityCritical]
		get
		{
			if (_properties == null)
			{
				_properties = new DictionaryWrapper(this, WrappedMessage.Properties);
			}
			return _properties;
		}
	}

	public virtual string TypeName
	{
		[SecurityCritical]
		get
		{
			return ((IMethodCallMessage)WrappedMessage).TypeName;
		}
	}

	public virtual string Uri
	{
		[SecurityCritical]
		get
		{
			return ((IMethodCallMessage)WrappedMessage).Uri;
		}
		set
		{
			if (WrappedMessage is IInternalMessage internalMessage)
			{
				internalMessage.Uri = value;
			}
			else
			{
				Properties["__Uri"] = value;
			}
		}
	}

	public MethodCallMessageWrapper(IMethodCallMessage msg)
		: base(msg)
	{
		_args = ((IMethodCallMessage)WrappedMessage).Args;
		_inArgInfo = new ArgInfo(msg.MethodBase, ArgInfoType.In);
	}

	[SecurityCritical]
	public virtual object GetArg(int argNum)
	{
		return _args[argNum];
	}

	[SecurityCritical]
	public virtual string GetArgName(int index)
	{
		return ((IMethodCallMessage)WrappedMessage).GetArgName(index);
	}

	[SecurityCritical]
	public virtual object GetInArg(int argNum)
	{
		return _args[_inArgInfo.GetInOutArgIndex(argNum)];
	}

	[SecurityCritical]
	public virtual string GetInArgName(int index)
	{
		return _inArgInfo.GetInOutArgName(index);
	}
}
