using System.Collections;
using System.Runtime.InteropServices;
using System.Security;

namespace System.Runtime.Remoting.Channels;

[Serializable]
[MonoTODO("Serialization format not compatible with .NET")]
[ComVisible(true)]
public class TransportHeaders : ITransportHeaders
{
	private Hashtable hash_table;

	public object this[object key]
	{
		[SecurityCritical]
		get
		{
			return hash_table[key];
		}
		[SecurityCritical]
		set
		{
			hash_table[key] = value;
		}
	}

	public TransportHeaders()
	{
		hash_table = new Hashtable(CaseInsensitiveHashCodeProvider.DefaultInvariant, CaseInsensitiveComparer.DefaultInvariant);
	}

	[SecurityCritical]
	public IEnumerator GetEnumerator()
	{
		return hash_table.GetEnumerator();
	}
}
