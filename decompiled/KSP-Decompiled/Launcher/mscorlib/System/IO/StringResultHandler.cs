using System.Security;

namespace System.IO;

internal class StringResultHandler : SearchResultHandler<string>
{
	private bool _includeFiles;

	private bool _includeDirs;

	internal StringResultHandler(bool includeFiles, bool includeDirs)
	{
		_includeFiles = includeFiles;
		_includeDirs = includeDirs;
	}

	[SecurityCritical]
	internal override bool IsResultIncluded(SearchResult result)
	{
		bool num = _includeFiles && FileSystemEnumerableHelpers.IsFile(result.FindData);
		bool flag = _includeDirs && FileSystemEnumerableHelpers.IsDir(result.FindData);
		return num || flag;
	}

	[SecurityCritical]
	internal override string CreateObject(SearchResult result)
	{
		return result.UserPath;
	}
}
