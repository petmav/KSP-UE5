using System.Reflection;
using System.Reflection.Emit;

namespace System.Xml.Xsl.IlGen;

internal struct StorageDescriptor
{
	private ItemLocation location;

	private object locationObject;

	private Type itemStorageType;

	private bool isCached;

	public ItemLocation Location => location;

	public int ParameterLocation => (int)locationObject;

	public LocalBuilder LocalLocation => locationObject as LocalBuilder;

	public LocalBuilder CurrentLocation => locationObject as LocalBuilder;

	public MethodInfo GlobalLocation => locationObject as MethodInfo;

	public bool IsCached => isCached;

	public Type ItemStorageType => itemStorageType;

	public static StorageDescriptor None()
	{
		return default(StorageDescriptor);
	}

	public static StorageDescriptor Stack(Type itemStorageType, bool isCached)
	{
		StorageDescriptor result = default(StorageDescriptor);
		result.location = ItemLocation.Stack;
		result.itemStorageType = itemStorageType;
		result.isCached = isCached;
		return result;
	}

	public static StorageDescriptor Parameter(int paramIndex, Type itemStorageType, bool isCached)
	{
		StorageDescriptor result = default(StorageDescriptor);
		result.location = ItemLocation.Parameter;
		result.locationObject = paramIndex;
		result.itemStorageType = itemStorageType;
		result.isCached = isCached;
		return result;
	}

	public static StorageDescriptor Local(LocalBuilder loc, Type itemStorageType, bool isCached)
	{
		StorageDescriptor result = default(StorageDescriptor);
		result.location = ItemLocation.Local;
		result.locationObject = loc;
		result.itemStorageType = itemStorageType;
		result.isCached = isCached;
		return result;
	}

	public static StorageDescriptor Current(LocalBuilder locIter, Type itemStorageType)
	{
		StorageDescriptor result = default(StorageDescriptor);
		result.location = ItemLocation.Current;
		result.locationObject = locIter;
		result.itemStorageType = itemStorageType;
		return result;
	}

	public static StorageDescriptor Global(MethodInfo methGlobal, Type itemStorageType, bool isCached)
	{
		StorageDescriptor result = default(StorageDescriptor);
		result.location = ItemLocation.Global;
		result.locationObject = methGlobal;
		result.itemStorageType = itemStorageType;
		result.isCached = isCached;
		return result;
	}

	public StorageDescriptor ToStack()
	{
		return Stack(itemStorageType, isCached);
	}

	public StorageDescriptor ToLocal(LocalBuilder loc)
	{
		return Local(loc, itemStorageType, isCached);
	}

	public StorageDescriptor ToStorageType(Type itemStorageType)
	{
		StorageDescriptor result = this;
		result.itemStorageType = itemStorageType;
		return result;
	}
}
