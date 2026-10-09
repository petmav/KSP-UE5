using System.Collections;
using System.Collections.Specialized;

namespace System.CodeDom;

[Serializable]
public class CodeObject
{
	private IDictionary _userData;

	public IDictionary UserData => _userData ?? (_userData = new ListDictionary());
}
