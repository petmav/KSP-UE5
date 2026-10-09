using System.Collections.Specialized;

namespace System.Net;

internal class TrackingStringDictionary : StringDictionary
{
	private bool isChanged;

	private bool isReadOnly;

	internal bool IsChanged
	{
		get
		{
			return isChanged;
		}
		set
		{
			isChanged = value;
		}
	}

	public override string this[string key]
	{
		get
		{
			return base[key];
		}
		set
		{
			if (isReadOnly)
			{
				throw new InvalidOperationException(global::SR.GetString("The collection is read-only."));
			}
			base[key] = value;
			isChanged = true;
		}
	}

	internal TrackingStringDictionary()
		: this(isReadOnly: false)
	{
	}

	internal TrackingStringDictionary(bool isReadOnly)
	{
		this.isReadOnly = isReadOnly;
	}

	public override void Add(string key, string value)
	{
		if (isReadOnly)
		{
			throw new InvalidOperationException(global::SR.GetString("The collection is read-only."));
		}
		base.Add(key, value);
		isChanged = true;
	}

	public override void Clear()
	{
		if (isReadOnly)
		{
			throw new InvalidOperationException(global::SR.GetString("The collection is read-only."));
		}
		base.Clear();
		isChanged = true;
	}

	public override void Remove(string key)
	{
		if (isReadOnly)
		{
			throw new InvalidOperationException(global::SR.GetString("The collection is read-only."));
		}
		base.Remove(key);
		isChanged = true;
	}
}
