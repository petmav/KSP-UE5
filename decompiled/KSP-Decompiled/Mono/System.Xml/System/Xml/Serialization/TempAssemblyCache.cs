using System.Collections;

namespace System.Xml.Serialization;

internal class TempAssemblyCache
{
	private Hashtable cache = new Hashtable();

	internal TempAssembly this[string ns, object o] => (TempAssembly)cache[new TempAssemblyCacheKey(ns, o)];

	internal void Add(string ns, object o, TempAssembly assembly)
	{
		TempAssemblyCacheKey key = new TempAssemblyCacheKey(ns, o);
		lock (this)
		{
			if (cache[key] == assembly)
			{
				return;
			}
			Hashtable hashtable = new Hashtable();
			foreach (object key2 in cache.Keys)
			{
				hashtable.Add(key2, cache[key2]);
			}
			cache = hashtable;
			cache[key] = assembly;
		}
	}
}
