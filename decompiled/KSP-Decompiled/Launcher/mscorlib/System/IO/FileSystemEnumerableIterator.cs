using System.Collections.Generic;
using System.Security;
using Microsoft.Win32;
using Microsoft.Win32.SafeHandles;

namespace System.IO;

internal class FileSystemEnumerableIterator<TSource> : Iterator<TSource>
{
	private const int STATE_INIT = 1;

	private const int STATE_SEARCH_NEXT_DIR = 2;

	private const int STATE_FIND_NEXT_FILE = 3;

	private const int STATE_FINISH = 4;

	private SearchResultHandler<TSource> _resultHandler;

	private List<Directory.SearchData> searchStack;

	private Directory.SearchData searchData;

	private string searchCriteria;

	[SecurityCritical]
	private SafeFindHandle _hnd;

	private bool needsParentPathDiscoveryDemand;

	private bool empty;

	private string userPath;

	private SearchOption searchOption;

	private string fullPath;

	private string normalizedSearchPath;

	private bool _checkHost;

	[SecuritySafeCritical]
	internal FileSystemEnumerableIterator(string path, string originalUserPath, string searchPattern, SearchOption searchOption, SearchResultHandler<TSource> resultHandler, bool checkHost)
	{
		searchStack = new List<Directory.SearchData>();
		string text = NormalizeSearchPattern(searchPattern);
		if (text.Length == 0)
		{
			empty = true;
			return;
		}
		_resultHandler = resultHandler;
		this.searchOption = searchOption;
		fullPath = Path.GetFullPathInternal(path);
		string fullSearchString = GetFullSearchString(fullPath, text);
		normalizedSearchPath = Path.GetDirectoryName(fullSearchString);
		_ = new string[2]
		{
			Directory.GetDemandDir(fullPath, thisDirOnly: true),
			Directory.GetDemandDir(normalizedSearchPath, thisDirOnly: true)
		};
		_checkHost = checkHost;
		searchCriteria = GetNormalizedSearchCriteria(fullSearchString, normalizedSearchPath);
		string directoryName = Path.GetDirectoryName(text);
		string path2 = originalUserPath;
		if (directoryName != null && directoryName.Length != 0)
		{
			path2 = Path.Combine(path2, directoryName);
		}
		userPath = path2;
		searchData = new Directory.SearchData(normalizedSearchPath, userPath, searchOption);
		CommonInit();
	}

	[SecurityCritical]
	private void CommonInit()
	{
		string pathWithPattern = Path.InternalCombine(searchData.fullPath, searchCriteria);
		Win32Native.WIN32_FIND_DATA wIN32_FIND_DATA = new Win32Native.WIN32_FIND_DATA();
		_hnd = new SafeFindHandle(MonoIO.FindFirstFile(pathWithPattern, out wIN32_FIND_DATA.cFileName, out wIN32_FIND_DATA.dwFileAttributes, out var error));
		if (_hnd.IsInvalid)
		{
			int num = error;
			if (num != 2 && num != 18)
			{
				HandleError(num, searchData.fullPath);
			}
			else
			{
				empty = searchData.searchOption == SearchOption.TopDirectoryOnly;
			}
		}
		if (searchData.searchOption == SearchOption.TopDirectoryOnly)
		{
			if (empty)
			{
				_hnd.Dispose();
				return;
			}
			SearchResult result = CreateSearchResult(searchData, wIN32_FIND_DATA);
			if (_resultHandler.IsResultIncluded(result))
			{
				current = _resultHandler.CreateObject(result);
			}
		}
		else
		{
			_hnd.Dispose();
			searchStack.Add(searchData);
		}
	}

	[SecuritySafeCritical]
	private FileSystemEnumerableIterator(string fullPath, string normalizedSearchPath, string searchCriteria, string userPath, SearchOption searchOption, SearchResultHandler<TSource> resultHandler, bool checkHost)
	{
		this.fullPath = fullPath;
		this.normalizedSearchPath = normalizedSearchPath;
		this.searchCriteria = searchCriteria;
		_resultHandler = resultHandler;
		this.userPath = userPath;
		this.searchOption = searchOption;
		_checkHost = checkHost;
		searchStack = new List<Directory.SearchData>();
		if (searchCriteria != null)
		{
			_ = new string[2]
			{
				Directory.GetDemandDir(fullPath, thisDirOnly: true),
				Directory.GetDemandDir(normalizedSearchPath, thisDirOnly: true)
			};
			searchData = new Directory.SearchData(normalizedSearchPath, userPath, searchOption);
			CommonInit();
		}
		else
		{
			empty = true;
		}
	}

	protected override Iterator<TSource> Clone()
	{
		return new FileSystemEnumerableIterator<TSource>(fullPath, normalizedSearchPath, searchCriteria, userPath, searchOption, _resultHandler, _checkHost);
	}

	[SecuritySafeCritical]
	protected override void Dispose(bool disposing)
	{
		try
		{
			if (_hnd != null)
			{
				_hnd.Dispose();
			}
		}
		finally
		{
			base.Dispose(disposing);
		}
	}

	[SecuritySafeCritical]
	public override bool MoveNext()
	{
		Win32Native.WIN32_FIND_DATA wIN32_FIND_DATA = new Win32Native.WIN32_FIND_DATA();
		switch (state)
		{
		case 1:
			if (empty)
			{
				state = 4;
				goto case 4;
			}
			if (searchData.searchOption == SearchOption.TopDirectoryOnly)
			{
				state = 3;
				if (current != null)
				{
					return true;
				}
				goto case 3;
			}
			state = 2;
			goto case 2;
		case 2:
			while (searchStack.Count > 0)
			{
				searchData = searchStack[0];
				searchStack.RemoveAt(0);
				AddSearchableDirsToStack(searchData);
				string pathWithPattern = Path.InternalCombine(searchData.fullPath, searchCriteria);
				_hnd = new SafeFindHandle(MonoIO.FindFirstFile(pathWithPattern, out wIN32_FIND_DATA.cFileName, out wIN32_FIND_DATA.dwFileAttributes, out var error2));
				if (_hnd.IsInvalid)
				{
					int num2 = error2;
					if (num2 == 2 || num2 == 18 || num2 == 3)
					{
						continue;
					}
					_hnd.Dispose();
					HandleError(num2, searchData.fullPath);
				}
				state = 3;
				needsParentPathDiscoveryDemand = true;
				SearchResult result2 = CreateSearchResult(searchData, wIN32_FIND_DATA);
				if (_resultHandler.IsResultIncluded(result2))
				{
					if (needsParentPathDiscoveryDemand)
					{
						DoDemand(searchData.fullPath);
						needsParentPathDiscoveryDemand = false;
					}
					current = _resultHandler.CreateObject(result2);
					return true;
				}
				goto case 3;
			}
			state = 4;
			goto case 4;
		case 3:
			if (searchData != null && _hnd != null)
			{
				int error;
				while (MonoIO.FindNextFile(_hnd.DangerousGetHandle(), out wIN32_FIND_DATA.cFileName, out wIN32_FIND_DATA.dwFileAttributes, out error))
				{
					SearchResult result = CreateSearchResult(searchData, wIN32_FIND_DATA);
					if (_resultHandler.IsResultIncluded(result))
					{
						if (needsParentPathDiscoveryDemand)
						{
							DoDemand(searchData.fullPath);
							needsParentPathDiscoveryDemand = false;
						}
						current = _resultHandler.CreateObject(result);
						return true;
					}
				}
				int num = error;
				if (_hnd != null)
				{
					_hnd.Dispose();
				}
				if (num != 0 && num != 18 && num != 2)
				{
					HandleError(num, searchData.fullPath);
				}
			}
			if (searchData.searchOption == SearchOption.TopDirectoryOnly)
			{
				state = 4;
				goto case 4;
			}
			state = 2;
			goto case 2;
		case 4:
			Dispose();
			break;
		}
		return false;
	}

	[SecurityCritical]
	private SearchResult CreateSearchResult(Directory.SearchData localSearchData, Win32Native.WIN32_FIND_DATA findData)
	{
		string text = Path.InternalCombine(localSearchData.userPath, findData.cFileName);
		return new SearchResult(Path.InternalCombine(localSearchData.fullPath, findData.cFileName), text, findData);
	}

	[SecurityCritical]
	private void HandleError(int hr, string path)
	{
		Dispose();
		__Error.WinIOError(hr, path);
	}

	[SecurityCritical]
	private void AddSearchableDirsToStack(Directory.SearchData localSearchData)
	{
		string pathWithPattern = Path.InternalCombine(localSearchData.fullPath, "*");
		SafeFindHandle safeFindHandle = null;
		Win32Native.WIN32_FIND_DATA wIN32_FIND_DATA = new Win32Native.WIN32_FIND_DATA();
		try
		{
			safeFindHandle = new SafeFindHandle(MonoIO.FindFirstFile(pathWithPattern, out wIN32_FIND_DATA.cFileName, out wIN32_FIND_DATA.dwFileAttributes, out var error));
			if (safeFindHandle.IsInvalid)
			{
				int num = error;
				if (num == 2 || num == 18 || num == 3)
				{
					return;
				}
				HandleError(num, localSearchData.fullPath);
			}
			int num2 = 0;
			do
			{
				if (FileSystemEnumerableHelpers.IsDir(wIN32_FIND_DATA))
				{
					string text = Path.InternalCombine(localSearchData.fullPath, wIN32_FIND_DATA.cFileName);
					string text2 = Path.InternalCombine(localSearchData.userPath, wIN32_FIND_DATA.cFileName);
					SearchOption searchOption = localSearchData.searchOption;
					Directory.SearchData item = new Directory.SearchData(text, text2, searchOption);
					searchStack.Insert(num2++, item);
				}
			}
			while (MonoIO.FindNextFile(safeFindHandle.DangerousGetHandle(), out wIN32_FIND_DATA.cFileName, out wIN32_FIND_DATA.dwFileAttributes, out error));
		}
		finally
		{
			safeFindHandle?.Dispose();
		}
	}

	[SecurityCritical]
	internal void DoDemand(string fullPathToDemand)
	{
	}

	private static string NormalizeSearchPattern(string searchPattern)
	{
		string text = searchPattern.TrimEnd(Path.TrimEndChars);
		if (text.Equals("."))
		{
			text = "*";
		}
		Path.CheckSearchPattern(text);
		return text;
	}

	private static string GetNormalizedSearchCriteria(string fullSearchString, string fullPathMod)
	{
		string text = null;
		if (Path.IsDirectorySeparator(fullPathMod[fullPathMod.Length - 1]))
		{
			return fullSearchString.Substring(fullPathMod.Length);
		}
		return fullSearchString.Substring(fullPathMod.Length + 1);
	}

	private static string GetFullSearchString(string fullPath, string searchPattern)
	{
		string text = Path.InternalCombine(fullPath, searchPattern);
		char c = text[text.Length - 1];
		if (Path.IsDirectorySeparator(c) || c == Path.VolumeSeparatorChar)
		{
			text += "*";
		}
		return text;
	}
}
