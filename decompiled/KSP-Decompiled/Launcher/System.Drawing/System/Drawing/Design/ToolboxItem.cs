using System.Collections;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Reflection;
using System.Runtime.Serialization;
using System.Security.Permissions;
using Unity;

namespace System.Drawing.Design;

[Serializable]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
public class ToolboxItem : ISerializable
{
	public AssemblyName AssemblyName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public Bitmap Bitmap
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public string Company
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public virtual string ComponentType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public AssemblyName[] DependentAssemblies
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public string Description
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public string DisplayName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public ICollection Filter
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool IsTransient
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public virtual bool Locked
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public Bitmap OriginalBitmap
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public IDictionary Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string TypeName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public virtual string Version
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public event ToolboxComponentsCreatedEventHandler ComponentsCreated
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public event ToolboxComponentsCreatingEventHandler ComponentsCreating
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public ToolboxItem()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ToolboxItem(Type toolType)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected void CheckUnlocked()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public IComponent[] CreateComponents()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public IComponent[] CreateComponents(IDesignerHost host)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public IComponent[] CreateComponents(IDesignerHost host, IDictionary defaultValues)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected virtual IComponent[] CreateComponentsCore(IDesignerHost host)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected virtual IComponent[] CreateComponentsCore(IDesignerHost host, IDictionary defaultValues)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected virtual void Deserialize(SerializationInfo info, StreamingContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual object FilterPropertyValue(string propertyName, object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public Type GetType(IDesignerHost host)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected virtual Type GetType(IDesignerHost host, AssemblyName assemblyName, string typeName, bool reference)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual void Initialize(Type type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void Lock()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OnComponentsCreated(ToolboxComponentsCreatedEventArgs args)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OnComponentsCreating(ToolboxComponentsCreatingEventArgs args)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void Serialize(SerializationInfo info, StreamingContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected void ValidatePropertyType(string propertyName, object value, Type expectedType, bool allowNull)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual object ValidatePropertyValue(string propertyName, object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
