using System.Collections.Generic;
using System.Reflection.Emit;

namespace System.Xml.Serialization;

internal class LocalScope
{
	public readonly LocalScope parent;

	private readonly Dictionary<string, LocalBuilder> locals;

	public LocalBuilder this[string key]
	{
		get
		{
			TryGetValue(key, out var value);
			return value;
		}
		set
		{
			locals[key] = value;
		}
	}

	public LocalScope()
	{
		locals = new Dictionary<string, LocalBuilder>();
	}

	public LocalScope(LocalScope parent)
		: this()
	{
		this.parent = parent;
	}

	public void Add(string key, LocalBuilder value)
	{
		locals.Add(key, value);
	}

	public bool ContainsKey(string key)
	{
		if (!locals.ContainsKey(key))
		{
			if (parent != null)
			{
				return parent.ContainsKey(key);
			}
			return false;
		}
		return true;
	}

	public bool TryGetValue(string key, out LocalBuilder value)
	{
		if (locals.TryGetValue(key, out value))
		{
			return true;
		}
		if (parent != null)
		{
			return parent.TryGetValue(key, out value);
		}
		value = null;
		return false;
	}

	public void AddToFreeLocals(Dictionary<Tuple<Type, string>, Queue<LocalBuilder>> freeLocals)
	{
		foreach (KeyValuePair<string, LocalBuilder> local in locals)
		{
			Tuple<Type, string> key = new Tuple<Type, string>(local.Value.LocalType, local.Key);
			if (freeLocals.TryGetValue(key, out var value))
			{
				value.Enqueue(local.Value);
				continue;
			}
			value = new Queue<LocalBuilder>();
			value.Enqueue(local.Value);
			freeLocals.Add(key, value);
		}
	}
}
