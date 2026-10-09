using System.Collections.Generic;
using System.Reflection.Emit;
using System.Xml.XPath;

namespace System.Xml.Xsl.IlGen;

internal class IteratorDescriptor
{
	private GenerateHelper helper;

	private IteratorDescriptor iterParent;

	private Label lblNext;

	private bool hasNext;

	private LocalBuilder locPos;

	private BranchingContext brctxt;

	private Label lblBranch;

	private StorageDescriptor storage;

	public IteratorDescriptor ParentIterator => iterParent;

	public bool HasLabelNext => hasNext;

	public LocalBuilder LocalPosition
	{
		get
		{
			return locPos;
		}
		set
		{
			locPos = value;
		}
	}

	public bool IsBranching => brctxt != BranchingContext.None;

	public Label LabelBranch => lblBranch;

	public BranchingContext CurrentBranchingContext => brctxt;

	public StorageDescriptor Storage
	{
		get
		{
			return storage;
		}
		set
		{
			storage = value;
		}
	}

	public IteratorDescriptor(GenerateHelper helper)
	{
		Init(null, helper);
	}

	public IteratorDescriptor(IteratorDescriptor iterParent)
	{
		Init(iterParent, iterParent.helper);
	}

	private void Init(IteratorDescriptor iterParent, GenerateHelper helper)
	{
		this.helper = helper;
		this.iterParent = iterParent;
	}

	public Label GetLabelNext()
	{
		return lblNext;
	}

	public void SetIterator(Label lblNext, StorageDescriptor storage)
	{
		this.lblNext = lblNext;
		hasNext = true;
		this.storage = storage;
	}

	public void SetIterator(IteratorDescriptor iterInfo)
	{
		if (iterInfo.HasLabelNext)
		{
			lblNext = iterInfo.GetLabelNext();
			hasNext = true;
		}
		storage = iterInfo.Storage;
	}

	public void LoopToEnd(Label lblOnEnd)
	{
		if (hasNext)
		{
			helper.BranchAndMark(lblNext, lblOnEnd);
			hasNext = false;
		}
		storage = StorageDescriptor.None();
	}

	public void CacheCount()
	{
		PushValue();
		helper.CallCacheCount(storage.ItemStorageType);
	}

	public void EnsureNoCache()
	{
		if (storage.IsCached)
		{
			if (!HasLabelNext)
			{
				EnsureStack();
				helper.LoadInteger(0);
				helper.CallCacheItem(storage.ItemStorageType);
				storage = StorageDescriptor.Stack(storage.ItemStorageType, isCached: false);
				return;
			}
			LocalBuilder locBldr = helper.DeclareLocal("$$$idx", typeof(int));
			EnsureNoStack("$$$cache");
			helper.LoadInteger(-1);
			helper.Emit(OpCodes.Stloc, locBldr);
			Label lbl = helper.DefineLabel();
			helper.MarkLabel(lbl);
			helper.Emit(OpCodes.Ldloc, locBldr);
			helper.LoadInteger(1);
			helper.Emit(OpCodes.Add);
			helper.Emit(OpCodes.Stloc, locBldr);
			helper.Emit(OpCodes.Ldloc, locBldr);
			CacheCount();
			helper.Emit(OpCodes.Bge, GetLabelNext());
			PushValue();
			helper.Emit(OpCodes.Ldloc, locBldr);
			helper.CallCacheItem(storage.ItemStorageType);
			SetIterator(lbl, StorageDescriptor.Stack(storage.ItemStorageType, isCached: false));
		}
	}

	public void SetBranching(BranchingContext brctxt, Label lblBranch)
	{
		this.brctxt = brctxt;
		this.lblBranch = lblBranch;
	}

	public void PushValue()
	{
		switch (storage.Location)
		{
		case ItemLocation.Stack:
			helper.Emit(OpCodes.Dup);
			break;
		case ItemLocation.Parameter:
			helper.LoadParameter(storage.ParameterLocation);
			break;
		case ItemLocation.Local:
			helper.Emit(OpCodes.Ldloc, storage.LocalLocation);
			break;
		case ItemLocation.Current:
			helper.Emit(OpCodes.Ldloca, storage.CurrentLocation);
			helper.Call(storage.CurrentLocation.LocalType.GetMethod("get_Current"));
			break;
		}
	}

	public void EnsureStack()
	{
		switch (storage.Location)
		{
		case ItemLocation.Stack:
			return;
		case ItemLocation.Parameter:
		case ItemLocation.Local:
		case ItemLocation.Current:
			PushValue();
			break;
		case ItemLocation.Global:
			helper.LoadQueryRuntime();
			helper.Call(storage.GlobalLocation);
			break;
		}
		storage = storage.ToStack();
	}

	public void EnsureNoStack(string locName)
	{
		if (storage.Location == ItemLocation.Stack)
		{
			EnsureLocal(locName);
		}
	}

	public void EnsureLocal(string locName)
	{
		if (storage.Location != ItemLocation.Local)
		{
			if (storage.IsCached)
			{
				EnsureLocal(helper.DeclareLocal(locName, typeof(IList<>).MakeGenericType(storage.ItemStorageType)));
			}
			else
			{
				EnsureLocal(helper.DeclareLocal(locName, storage.ItemStorageType));
			}
		}
	}

	public void EnsureLocal(LocalBuilder bldr)
	{
		if (storage.LocalLocation != bldr)
		{
			EnsureStack();
			helper.Emit(OpCodes.Stloc, bldr);
			storage = storage.ToLocal(bldr);
		}
	}

	public void DiscardStack()
	{
		if (storage.Location == ItemLocation.Stack)
		{
			helper.Emit(OpCodes.Pop);
			storage = StorageDescriptor.None();
		}
	}

	public void EnsureStackNoCache()
	{
		EnsureNoCache();
		EnsureStack();
	}

	public void EnsureNoStackNoCache(string locName)
	{
		EnsureNoCache();
		EnsureNoStack(locName);
	}

	public void EnsureLocalNoCache(string locName)
	{
		EnsureNoCache();
		EnsureLocal(locName);
	}

	public void EnsureLocalNoCache(LocalBuilder bldr)
	{
		EnsureNoCache();
		EnsureLocal(bldr);
	}

	public void EnsureItemStorageType(XmlQueryType xmlType, Type storageTypeDest)
	{
		if (!(storage.ItemStorageType == storageTypeDest))
		{
			if (!storage.IsCached)
			{
				goto IL_0087;
			}
			if (storage.ItemStorageType == typeof(XPathNavigator))
			{
				EnsureStack();
				helper.Call(XmlILMethods.NavsToItems);
			}
			else
			{
				if (!(storageTypeDest == typeof(XPathNavigator)))
				{
					goto IL_0087;
				}
				EnsureStack();
				helper.Call(XmlILMethods.ItemsToNavs);
			}
		}
		goto IL_014d;
		IL_014d:
		storage = storage.ToStorageType(storageTypeDest);
		return;
		IL_0087:
		EnsureStackNoCache();
		if (storage.ItemStorageType == typeof(XPathItem))
		{
			if (storageTypeDest == typeof(XPathNavigator))
			{
				helper.Emit(OpCodes.Castclass, typeof(XPathNavigator));
			}
			else
			{
				helper.CallValueAs(storageTypeDest);
			}
		}
		else if (!(storage.ItemStorageType == typeof(XPathNavigator)))
		{
			helper.LoadInteger(helper.StaticData.DeclareXmlType(xmlType));
			helper.LoadQueryRuntime();
			helper.Call(XmlILMethods.StorageMethods[storage.ItemStorageType].ToAtomicValue);
		}
		goto IL_014d;
	}
}
