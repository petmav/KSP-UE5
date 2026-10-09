using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Security.Permissions;
using System.Text;

namespace System.IO;

[ComVisible(true)]
public static class Path
{
	[Obsolete("see GetInvalidPathChars and GetInvalidFileNameChars methods.")]
	public static readonly char[] InvalidPathChars;

	public static readonly char AltDirectorySeparatorChar;

	public static readonly char DirectorySeparatorChar;

	public static readonly char PathSeparator;

	internal static readonly string DirectorySeparatorStr;

	public static readonly char VolumeSeparatorChar;

	internal static readonly char[] PathSeparatorChars;

	private static readonly bool dirEqualsVolume;

	internal const int MAX_PATH = 260;

	internal static readonly char[] trimEndCharsWindows;

	internal static readonly char[] trimEndCharsUnix;

	internal static string DirectorySeparatorCharAsString => DirectorySeparatorStr;

	internal static char[] TrimEndChars
	{
		get
		{
			if (!Environment.IsRunningOnWindows)
			{
				return trimEndCharsUnix;
			}
			return trimEndCharsWindows;
		}
	}

	public static string ChangeExtension(string path, string extension)
	{
		if (path == null)
		{
			return null;
		}
		if (path.IndexOfAny(InvalidPathChars) != -1)
		{
			throw new ArgumentException("Illegal characters in path.");
		}
		int num = findExtension(path);
		if (extension == null)
		{
			if (num >= 0)
			{
				return path.Substring(0, num);
			}
			return path;
		}
		if (extension.Length == 0)
		{
			if (num >= 0)
			{
				return path.Substring(0, num + 1);
			}
			return path + ".";
		}
		if (path.Length != 0)
		{
			if (extension.Length > 0 && extension[0] != '.')
			{
				extension = "." + extension;
			}
		}
		else
		{
			extension = string.Empty;
		}
		if (num < 0)
		{
			return path + extension;
		}
		if (num > 0)
		{
			return path.Substring(0, num) + extension;
		}
		return extension;
	}

	public static string Combine(string path1, string path2)
	{
		if (path1 == null)
		{
			throw new ArgumentNullException("path1");
		}
		if (path2 == null)
		{
			throw new ArgumentNullException("path2");
		}
		if (path1.Length == 0)
		{
			return path2;
		}
		if (path2.Length == 0)
		{
			return path1;
		}
		if (path1.IndexOfAny(InvalidPathChars) != -1)
		{
			throw new ArgumentException("Illegal characters in path.");
		}
		if (path2.IndexOfAny(InvalidPathChars) != -1)
		{
			throw new ArgumentException("Illegal characters in path.");
		}
		if (IsPathRooted(path2))
		{
			return path2;
		}
		char c = path1[path1.Length - 1];
		if (c != DirectorySeparatorChar && c != AltDirectorySeparatorChar && c != VolumeSeparatorChar)
		{
			return path1 + DirectorySeparatorStr + path2;
		}
		return path1 + path2;
	}

	internal static string CleanPath(string s)
	{
		int length = s.Length;
		int num = 0;
		int num2 = 0;
		int num3 = 0;
		char c = s[0];
		if (length > 2 && c == '\\' && s[1] == '\\')
		{
			num3 = 2;
		}
		if (length == 1 && (c == DirectorySeparatorChar || c == AltDirectorySeparatorChar))
		{
			return s;
		}
		for (int i = num3; i < length; i++)
		{
			char c2 = s[i];
			if (c2 != DirectorySeparatorChar && c2 != AltDirectorySeparatorChar)
			{
				continue;
			}
			if (DirectorySeparatorChar != AltDirectorySeparatorChar && c2 == AltDirectorySeparatorChar)
			{
				num2++;
			}
			if (i + 1 == length)
			{
				num++;
				continue;
			}
			c2 = s[i + 1];
			if (c2 == DirectorySeparatorChar || c2 == AltDirectorySeparatorChar)
			{
				num++;
			}
		}
		if (num == 0 && num2 == 0)
		{
			return s;
		}
		char[] array = new char[length - num];
		if (num3 != 0)
		{
			array[0] = '\\';
			array[1] = '\\';
		}
		int j = num3;
		int num4 = num3;
		for (; j < length; j++)
		{
			if (num4 >= array.Length)
			{
				break;
			}
			char c3 = s[j];
			if (c3 != DirectorySeparatorChar && c3 != AltDirectorySeparatorChar)
			{
				array[num4++] = c3;
			}
			else
			{
				if (num4 + 1 == array.Length)
				{
					continue;
				}
				array[num4++] = DirectorySeparatorChar;
				for (; j < length - 1; j++)
				{
					c3 = s[j + 1];
					if (c3 != DirectorySeparatorChar && c3 != AltDirectorySeparatorChar)
					{
						break;
					}
				}
			}
		}
		return new string(array);
	}

	public static string GetDirectoryName(string path)
	{
		if (path == string.Empty)
		{
			throw new ArgumentException("Invalid path");
		}
		if (path == null || GetPathRoot(path) == path)
		{
			return null;
		}
		if (path.Trim().Length == 0)
		{
			throw new ArgumentException("Argument string consists of whitespace characters only.");
		}
		if (path.IndexOfAny(InvalidPathChars) > -1)
		{
			throw new ArgumentException("Path contains invalid characters");
		}
		int num = path.LastIndexOfAny(PathSeparatorChars);
		if (num == 0)
		{
			num++;
		}
		if (num > 0)
		{
			string text = path.Substring(0, num);
			int length = text.Length;
			if (length >= 2 && DirectorySeparatorChar == '\\' && text[length - 1] == VolumeSeparatorChar)
			{
				return text + DirectorySeparatorChar;
			}
			if (length == 1 && DirectorySeparatorChar == '\\' && path.Length >= 2 && path[num] == VolumeSeparatorChar)
			{
				return text + VolumeSeparatorChar;
			}
			return CleanPath(text);
		}
		return string.Empty;
	}

	public static string GetExtension(string path)
	{
		if (path == null)
		{
			return null;
		}
		if (path.IndexOfAny(InvalidPathChars) != -1)
		{
			throw new ArgumentException("Illegal characters in path.");
		}
		int num = findExtension(path);
		if (num > -1 && num < path.Length - 1)
		{
			return path.Substring(num);
		}
		return string.Empty;
	}

	public static string GetFileName(string path)
	{
		if (path == null || path.Length == 0)
		{
			return path;
		}
		if (path.IndexOfAny(InvalidPathChars) != -1)
		{
			throw new ArgumentException("Illegal characters in path.");
		}
		int num = path.LastIndexOfAny(PathSeparatorChars);
		if (num >= 0)
		{
			return path.Substring(num + 1);
		}
		return path;
	}

	public static string GetFileNameWithoutExtension(string path)
	{
		return ChangeExtension(GetFileName(path), null);
	}

	public static string GetFullPath(string path)
	{
		return InsecureGetFullPath(path);
	}

	internal static string GetFullPathInternal(string path)
	{
		return InsecureGetFullPath(path);
	}

	[DllImport("Kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
	private static extern int GetFullPathName(string path, int numBufferChars, StringBuilder buffer, ref IntPtr lpFilePartOrNull);

	internal static string GetFullPathName(string path)
	{
		StringBuilder stringBuilder = new StringBuilder(260);
		IntPtr lpFilePartOrNull = IntPtr.Zero;
		int fullPathName = GetFullPathName(path, 260, stringBuilder, ref lpFilePartOrNull);
		if (fullPathName == 0)
		{
			int lastWin32Error = Marshal.GetLastWin32Error();
			throw new IOException("Windows API call to GetFullPathName failed, Windows error code: " + lastWin32Error);
		}
		if (fullPathName > 260)
		{
			stringBuilder = new StringBuilder(fullPathName);
			GetFullPathName(path, fullPathName, stringBuilder, ref lpFilePartOrNull);
		}
		return stringBuilder.ToString();
	}

	internal static string WindowsDriveAdjustment(string path)
	{
		if (path.Length < 2)
		{
			if (path.Length == 1 && (path[0] == '\\' || path[0] == '/'))
			{
				return GetPathRoot(Directory.GetCurrentDirectory());
			}
			return path;
		}
		if (path[1] != ':' || !char.IsLetter(path[0]))
		{
			return path;
		}
		string text = Directory.InsecureGetCurrentDirectory();
		if (path.Length == 2)
		{
			path = ((text[0] != path[0]) ? GetFullPathName(path) : text);
		}
		else if (path[2] != DirectorySeparatorChar && path[2] != AltDirectorySeparatorChar)
		{
			path = ((text[0] != path[0]) ? GetFullPathName(path) : Combine(text, path.Substring(2, path.Length - 2)));
		}
		return path;
	}

	internal static string InsecureGetFullPath(string path)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (path.Trim().Length == 0)
		{
			throw new ArgumentException(Locale.GetText("The specified path is not of a legal form (empty)."));
		}
		if (Environment.IsRunningOnWindows)
		{
			path = WindowsDriveAdjustment(path);
		}
		char c = path[path.Length - 1];
		bool flag = true;
		if (path.Length >= 2 && IsDirectorySeparator(path[0]) && IsDirectorySeparator(path[1]))
		{
			if (path.Length == 2 || path.IndexOf(path[0], 2) < 0)
			{
				throw new ArgumentException("UNC paths should be of the form \\\\server\\share.");
			}
			if (path[0] != DirectorySeparatorChar)
			{
				path = path.Replace(AltDirectorySeparatorChar, DirectorySeparatorChar);
			}
		}
		else if (!IsPathRooted(path))
		{
			if (!Environment.IsRunningOnWindows)
			{
				int num = 0;
				while ((num = path.IndexOf('.', num)) != -1 && ++num != path.Length && path[num] != DirectorySeparatorChar && path[num] != AltDirectorySeparatorChar)
				{
				}
				flag = num > 0;
			}
			string text = Directory.InsecureGetCurrentDirectory();
			path = ((text[text.Length - 1] != DirectorySeparatorChar) ? (text + DirectorySeparatorChar + path) : (text + path));
		}
		else if (DirectorySeparatorChar == '\\' && path.Length >= 2 && IsDirectorySeparator(path[0]) && !IsDirectorySeparator(path[1]))
		{
			string text2 = Directory.InsecureGetCurrentDirectory();
			path = ((text2[1] != VolumeSeparatorChar) ? text2.Substring(0, text2.IndexOf('\\', text2.IndexOfUnchecked("\\\\", 0, text2.Length) + 1)) : (text2.Substring(0, 2) + path));
		}
		if (flag)
		{
			path = CanonicalizePath(path);
		}
		if (IsDirectorySeparator(c) && path[path.Length - 1] != DirectorySeparatorChar)
		{
			path += DirectorySeparatorChar;
		}
		if (MonoIO.RemapPath(path, out var newPath))
		{
			path = newPath;
		}
		return path;
	}

	internal static bool IsDirectorySeparator(char c)
	{
		if (c != DirectorySeparatorChar)
		{
			return c == AltDirectorySeparatorChar;
		}
		return true;
	}

	public static string GetPathRoot(string path)
	{
		if (path == null)
		{
			return null;
		}
		if (path.Trim().Length == 0)
		{
			throw new ArgumentException("The specified path is not of a legal form.");
		}
		if (!IsPathRooted(path))
		{
			return string.Empty;
		}
		if (DirectorySeparatorChar == '/')
		{
			if (!IsDirectorySeparator(path[0]))
			{
				return string.Empty;
			}
			return DirectorySeparatorStr;
		}
		int i = 2;
		if (path.Length == 1 && IsDirectorySeparator(path[0]))
		{
			return DirectorySeparatorStr;
		}
		if (path.Length < 2)
		{
			return string.Empty;
		}
		if (IsDirectorySeparator(path[0]) && IsDirectorySeparator(path[1]))
		{
			for (; i < path.Length && !IsDirectorySeparator(path[i]); i++)
			{
			}
			if (i < path.Length)
			{
				for (i++; i < path.Length && !IsDirectorySeparator(path[i]); i++)
				{
				}
			}
			return DirectorySeparatorStr + DirectorySeparatorStr + path.Substring(2, i - 2).Replace(AltDirectorySeparatorChar, DirectorySeparatorChar);
		}
		if (IsDirectorySeparator(path[0]))
		{
			return DirectorySeparatorStr;
		}
		if (path[1] == VolumeSeparatorChar)
		{
			if (path.Length >= 3 && IsDirectorySeparator(path[2]))
			{
				i++;
			}
			return path.Substring(0, i);
		}
		return Directory.GetCurrentDirectory().Substring(0, 2);
	}

	[FileIOPermission(SecurityAction.Assert, Unrestricted = true)]
	public static string GetTempFileName()
	{
		FileStream fileStream = null;
		int num = 0;
		Random random = new Random();
		string tempPath = GetTempPath();
		string text;
		do
		{
			int num2 = random.Next();
			text = Combine(tempPath, "tmp" + (num2 + 1).ToString("x", CultureInfo.InvariantCulture) + ".tmp");
			try
			{
				fileStream = new FileStream(text, FileMode.CreateNew, FileAccess.ReadWrite, FileShare.Read, 8192, anonymous: false, (FileOptions)1);
			}
			catch (IOException ex)
			{
				if (ex._HResult != -2147024816 || num++ > 65536)
				{
					throw;
				}
			}
			catch (UnauthorizedAccessException ex2)
			{
				if (num++ > 65536)
				{
					throw new IOException(ex2.Message, ex2);
				}
			}
		}
		while (fileStream == null);
		fileStream.Close();
		return text;
	}

	[EnvironmentPermission(SecurityAction.Demand, Unrestricted = true)]
	public static string GetTempPath()
	{
		string temp_path = get_temp_path();
		if (temp_path.Length > 0 && temp_path[temp_path.Length - 1] != DirectorySeparatorChar)
		{
			return temp_path + DirectorySeparatorChar;
		}
		return temp_path;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern string get_temp_path();

	public static bool HasExtension(string path)
	{
		if (path == null || path.Trim().Length == 0)
		{
			return false;
		}
		if (path.IndexOfAny(InvalidPathChars) != -1)
		{
			throw new ArgumentException("Illegal characters in path.");
		}
		int num = findExtension(path);
		if (0 <= num)
		{
			return num < path.Length - 1;
		}
		return false;
	}

	public static bool IsPathRooted(string path)
	{
		if (path == null || path.Length == 0)
		{
			return false;
		}
		if (path.IndexOfAny(InvalidPathChars) != -1)
		{
			throw new ArgumentException("Illegal characters in path.");
		}
		char c = path[0];
		if (c != DirectorySeparatorChar && c != AltDirectorySeparatorChar)
		{
			if (!dirEqualsVolume && path.Length > 1)
			{
				return path[1] == VolumeSeparatorChar;
			}
			return false;
		}
		return true;
	}

	public static char[] GetInvalidFileNameChars()
	{
		if (!Environment.IsRunningOnWindows)
		{
			return new char[2] { '\0', '/' };
		}
		return new char[41]
		{
			'\0', '\u0001', '\u0002', '\u0003', '\u0004', '\u0005', '\u0006', '\a', '\b', '\t',
			'\n', '\v', '\f', '\r', '\u000e', '\u000f', '\u0010', '\u0011', '\u0012', '\u0013',
			'\u0014', '\u0015', '\u0016', '\u0017', '\u0018', '\u0019', '\u001a', '\u001b', '\u001c', '\u001d',
			'\u001e', '\u001f', '"', '<', '>', '|', ':', '*', '?', '\\',
			'/'
		};
	}

	public static char[] GetInvalidPathChars()
	{
		if (Environment.IsRunningOnWindows)
		{
			return new char[36]
			{
				'"', '<', '>', '|', '\0', '\u0001', '\u0002', '\u0003', '\u0004', '\u0005',
				'\u0006', '\a', '\b', '\t', '\n', '\v', '\f', '\r', '\u000e', '\u000f',
				'\u0010', '\u0011', '\u0012', '\u0013', '\u0014', '\u0015', '\u0016', '\u0017', '\u0018', '\u0019',
				'\u001a', '\u001b', '\u001c', '\u001d', '\u001e', '\u001f'
			};
		}
		return new char[1];
	}

	public static string GetRandomFileName()
	{
		StringBuilder stringBuilder = new StringBuilder(12);
		RandomNumberGenerator randomNumberGenerator = RandomNumberGenerator.Create();
		byte[] array = new byte[11];
		randomNumberGenerator.GetBytes(array);
		for (int i = 0; i < array.Length; i++)
		{
			if (stringBuilder.Length == 8)
			{
				stringBuilder.Append('.');
			}
			int num = array[i] % 36;
			char value = (char)((num < 26) ? (num + 97) : (num - 26 + 48));
			stringBuilder.Append(value);
		}
		return stringBuilder.ToString();
	}

	private static int findExtension(string path)
	{
		if (path != null)
		{
			int num = path.LastIndexOf('.');
			int num2 = path.LastIndexOfAny(PathSeparatorChars);
			if (num > num2)
			{
				return num;
			}
		}
		return -1;
	}

	static Path()
	{
		trimEndCharsWindows = new char[8] { '\t', '\n', '\v', '\f', '\r', ' ', '\u0085', '\u00a0' };
		trimEndCharsUnix = new char[0];
		VolumeSeparatorChar = MonoIO.VolumeSeparatorChar;
		DirectorySeparatorChar = MonoIO.DirectorySeparatorChar;
		AltDirectorySeparatorChar = MonoIO.AltDirectorySeparatorChar;
		PathSeparator = MonoIO.PathSeparator;
		InvalidPathChars = GetInvalidPathChars();
		DirectorySeparatorStr = DirectorySeparatorChar.ToString();
		PathSeparatorChars = new char[3] { DirectorySeparatorChar, AltDirectorySeparatorChar, VolumeSeparatorChar };
		dirEqualsVolume = DirectorySeparatorChar == VolumeSeparatorChar;
	}

	private static string GetServerAndShare(string path)
	{
		int i;
		for (i = 2; i < path.Length && !IsDirectorySeparator(path[i]); i++)
		{
		}
		if (i < path.Length)
		{
			for (i++; i < path.Length && !IsDirectorySeparator(path[i]); i++)
			{
			}
		}
		return path.Substring(2, i - 2).Replace(AltDirectorySeparatorChar, DirectorySeparatorChar);
	}

	private static bool SameRoot(string root, string path)
	{
		if (root.Length < 2 || path.Length < 2)
		{
			return false;
		}
		if (IsDirectorySeparator(root[0]) && IsDirectorySeparator(root[1]))
		{
			if (!IsDirectorySeparator(path[0]) || !IsDirectorySeparator(path[1]))
			{
				return false;
			}
			string serverAndShare = GetServerAndShare(root);
			string serverAndShare2 = GetServerAndShare(path);
			return string.Compare(serverAndShare, serverAndShare2, ignoreCase: true, CultureInfo.InvariantCulture) == 0;
		}
		if (!root[0].Equals(path[0]))
		{
			return false;
		}
		if (path[1] != VolumeSeparatorChar)
		{
			return false;
		}
		if (root.Length > 2 && path.Length > 2)
		{
			if (IsDirectorySeparator(root[2]))
			{
				return IsDirectorySeparator(path[2]);
			}
			return false;
		}
		return true;
	}

	private static string CanonicalizePath(string path)
	{
		if (path == null)
		{
			return path;
		}
		if (Environment.IsRunningOnWindows)
		{
			path = path.Trim();
		}
		if (path.Length == 0)
		{
			return path;
		}
		string pathRoot = GetPathRoot(path);
		string[] array = path.Split(DirectorySeparatorChar, AltDirectorySeparatorChar);
		int num = 0;
		bool flag = Environment.IsRunningOnWindows && pathRoot.Length > 2 && IsDirectorySeparator(pathRoot[0]) && IsDirectorySeparator(pathRoot[1]);
		int num2 = (flag ? 3 : 0);
		for (int i = 0; i < array.Length; i++)
		{
			if (Environment.IsRunningOnWindows)
			{
				array[i] = array[i].TrimEnd();
			}
			if (array[i] == "." || (i != 0 && array[i].Length == 0))
			{
				continue;
			}
			if (array[i] == "..")
			{
				if (num > num2)
				{
					num--;
				}
			}
			else
			{
				array[num++] = array[i];
			}
		}
		if (num == 0 || (num == 1 && array[0] == ""))
		{
			return pathRoot;
		}
		string text = string.Join(DirectorySeparatorStr, array, 0, num);
		if (Environment.IsRunningOnWindows)
		{
			if (flag)
			{
				text = DirectorySeparatorStr + text;
			}
			if (!SameRoot(pathRoot, text))
			{
				text = pathRoot + text;
			}
			if (flag)
			{
				return text;
			}
			if (!IsDirectorySeparator(path[0]) && SameRoot(pathRoot, path))
			{
				if (text.Length <= 2 && !text.EndsWith(DirectorySeparatorStr))
				{
					text += DirectorySeparatorChar;
				}
				return text;
			}
			string currentDirectory = Directory.GetCurrentDirectory();
			if (currentDirectory.Length > 1 && currentDirectory[1] == VolumeSeparatorChar)
			{
				if (text.Length == 0 || IsDirectorySeparator(text[0]))
				{
					text += "\\";
				}
				return currentDirectory.Substring(0, 2) + text;
			}
			if (IsDirectorySeparator(currentDirectory[currentDirectory.Length - 1]) && IsDirectorySeparator(text[0]))
			{
				return currentDirectory + text.Substring(1);
			}
			return currentDirectory + text;
		}
		if (pathRoot != "" && text.Length > 0 && text[0] != '/')
		{
			text = pathRoot + text;
		}
		return text;
	}

	internal static bool IsPathSubsetOf(string subset, string path)
	{
		if (subset.Length > path.Length)
		{
			return false;
		}
		int num = subset.LastIndexOfAny(PathSeparatorChars);
		if (string.Compare(subset, 0, path, 0, num) != 0)
		{
			return false;
		}
		num++;
		int num2 = path.IndexOfAny(PathSeparatorChars, num);
		if (num2 >= num)
		{
			return string.Compare(subset, num, path, num, path.Length - num2) == 0;
		}
		if (subset.Length != path.Length)
		{
			return false;
		}
		return string.Compare(subset, num, path, num, subset.Length - num) == 0;
	}

	public static string Combine(params string[] paths)
	{
		if (paths == null)
		{
			throw new ArgumentNullException("paths");
		}
		StringBuilder stringBuilder = new StringBuilder();
		int num = paths.Length;
		bool flag = false;
		foreach (string text in paths)
		{
			if (text == null)
			{
				throw new ArgumentNullException("One of the paths contains a null value", "paths");
			}
			if (text.Length == 0)
			{
				continue;
			}
			if (text.IndexOfAny(InvalidPathChars) != -1)
			{
				throw new ArgumentException("Illegal characters in path.");
			}
			if (flag)
			{
				flag = false;
				stringBuilder.Append(DirectorySeparatorStr);
			}
			num--;
			if (IsPathRooted(text))
			{
				stringBuilder.Length = 0;
			}
			stringBuilder.Append(text);
			int length = text.Length;
			if (length > 0 && num > 0)
			{
				char c = text[length - 1];
				if (c != DirectorySeparatorChar && c != AltDirectorySeparatorChar && c != VolumeSeparatorChar)
				{
					flag = true;
				}
			}
		}
		return stringBuilder.ToString();
	}

	public static string Combine(string path1, string path2, string path3)
	{
		if (path1 == null)
		{
			throw new ArgumentNullException("path1");
		}
		if (path2 == null)
		{
			throw new ArgumentNullException("path2");
		}
		if (path3 == null)
		{
			throw new ArgumentNullException("path3");
		}
		return Combine(new string[3] { path1, path2, path3 });
	}

	public static string Combine(string path1, string path2, string path3, string path4)
	{
		if (path1 == null)
		{
			throw new ArgumentNullException("path1");
		}
		if (path2 == null)
		{
			throw new ArgumentNullException("path2");
		}
		if (path3 == null)
		{
			throw new ArgumentNullException("path3");
		}
		if (path4 == null)
		{
			throw new ArgumentNullException("path4");
		}
		return Combine(new string[4] { path1, path2, path3, path4 });
	}

	internal static void Validate(string path)
	{
		Validate(path, "path");
	}

	internal static void Validate(string path, string parameterName)
	{
		if (path == null)
		{
			throw new ArgumentNullException(parameterName);
		}
		if (string.IsNullOrWhiteSpace(path))
		{
			throw new ArgumentException(Locale.GetText("Path is empty"));
		}
		if (path.IndexOfAny(InvalidPathChars) != -1)
		{
			throw new ArgumentException(Locale.GetText("Path contains invalid chars"));
		}
		if (Environment.IsRunningOnWindows)
		{
			int num = path.IndexOf(':');
			if (num >= 0 && num != 1)
			{
				throw new ArgumentException(parameterName);
			}
		}
	}

	internal static void CheckSearchPattern(string searchPattern)
	{
		int num;
		while ((num = searchPattern.IndexOf("..", StringComparison.Ordinal)) != -1)
		{
			if (num + 2 == searchPattern.Length)
			{
				throw new ArgumentException(Environment.GetResourceString("Search pattern cannot contain \"..\" to move up directories and can be contained only internally in file/directory names, as in \"a..b\"."));
			}
			if (searchPattern[num + 2] == DirectorySeparatorChar || searchPattern[num + 2] == AltDirectorySeparatorChar)
			{
				throw new ArgumentException(Environment.GetResourceString("Search pattern cannot contain \"..\" to move up directories and can be contained only internally in file/directory names, as in \"a..b\"."));
			}
			searchPattern = searchPattern.Substring(num + 2);
		}
	}

	internal static void CheckInvalidPathChars(string path, bool checkAdditional = false)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (PathInternal.HasIllegalCharacters(path, checkAdditional))
		{
			throw new ArgumentException(Environment.GetResourceString("Illegal characters in path."));
		}
	}

	internal static string InternalCombine(string path1, string path2)
	{
		if (path1 == null || path2 == null)
		{
			throw new ArgumentNullException((path1 == null) ? "path1" : "path2");
		}
		CheckInvalidPathChars(path1);
		CheckInvalidPathChars(path2);
		if (path2.Length == 0)
		{
			throw new ArgumentException(Environment.GetResourceString("Path cannot be the empty string or all whitespace."), "path2");
		}
		if (IsPathRooted(path2))
		{
			throw new ArgumentException(Environment.GetResourceString("Second path fragment must not be a drive or UNC name."), "path2");
		}
		int length = path1.Length;
		if (length == 0)
		{
			return path2;
		}
		char c = path1[length - 1];
		if (c != DirectorySeparatorChar && c != AltDirectorySeparatorChar && c != VolumeSeparatorChar)
		{
			return path1 + DirectorySeparatorCharAsString + path2;
		}
		return path1 + path2;
	}
}
