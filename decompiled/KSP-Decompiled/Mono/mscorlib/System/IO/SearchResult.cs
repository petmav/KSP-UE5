using System.Security;
using Microsoft.Win32;

namespace System.IO;

internal sealed class SearchResult
{
	private string fullPath;

	private string userPath;

	[SecurityCritical]
	private Win32Native.WIN32_FIND_DATA findData;

	internal string FullPath => fullPath;

	internal string UserPath => userPath;

	internal Win32Native.WIN32_FIND_DATA FindData
	{
		[SecurityCritical]
		get
		{
			return findData;
		}
	}

	[SecurityCritical]
	internal SearchResult(string fullPath, string userPath, Win32Native.WIN32_FIND_DATA findData)
	{
		this.fullPath = fullPath;
		this.userPath = userPath;
		this.findData = findData;
	}
}
