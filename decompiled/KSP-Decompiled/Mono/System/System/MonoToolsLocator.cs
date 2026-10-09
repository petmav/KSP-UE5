using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;

namespace System;

internal static class MonoToolsLocator
{
	public static readonly string Mono;

	public static readonly string McsCSharpCompiler;

	public static readonly string VBCompiler;

	public static readonly string AssemblyLinker;

	static MonoToolsLocator()
	{
		string directoryName = Path.GetDirectoryName((string)typeof(Environment).GetProperty("GacPath", BindingFlags.Static | BindingFlags.NonPublic).GetGetMethod(nonPublic: true).Invoke(null, null));
		if (Path.DirectorySeparatorChar == '\\')
		{
			StringBuilder stringBuilder = new StringBuilder(1024);
			GetModuleFileName(IntPtr.Zero, stringBuilder, stringBuilder.Capacity);
			string text = stringBuilder.ToString();
			string fileName = Path.GetFileName(text);
			if (fileName.StartsWith("mono") && fileName.EndsWith(".exe"))
			{
				Mono = text;
			}
			if (!File.Exists(Mono))
			{
				Mono = Path.Combine(Path.GetDirectoryName(Path.GetDirectoryName(directoryName)), "bin\\mono.exe");
			}
			if (!File.Exists(Mono))
			{
				Mono = Path.Combine(Path.GetDirectoryName(Path.GetDirectoryName(Path.GetDirectoryName(directoryName))), "mono\\mini\\mono.exe");
			}
			McsCSharpCompiler = Path.Combine(directoryName, "4.5", "mcs.exe");
			if (!File.Exists(McsCSharpCompiler))
			{
				McsCSharpCompiler = Path.Combine(Path.GetDirectoryName(directoryName), "lib", "net_4_x", "mcs.exe");
			}
			VBCompiler = Path.Combine(directoryName, "4.5\\vbnc.exe");
			AssemblyLinker = Path.Combine(directoryName, "4.5\\al.exe");
			if (!File.Exists(AssemblyLinker))
			{
				AssemblyLinker = Path.Combine(Path.GetDirectoryName(directoryName), "lib\\net_4_x\\al.exe");
			}
			return;
		}
		Mono = Path.Combine(directoryName, "bin", "mono");
		if (!File.Exists(Mono))
		{
			Mono = "mono";
		}
		string localPath = new Uri(typeof(object).Assembly.CodeBase).LocalPath;
		McsCSharpCompiler = Path.GetFullPath(Path.Combine(localPath, "..", "..", "..", "..", "bin", "mcs"));
		if (!File.Exists(McsCSharpCompiler))
		{
			McsCSharpCompiler = "mcs";
		}
		VBCompiler = Path.GetFullPath(Path.Combine(localPath, "..", "..", "..", "..", "bin", "vbnc"));
		if (!File.Exists(VBCompiler))
		{
			VBCompiler = "vbnc";
		}
		AssemblyLinker = Path.GetFullPath(Path.Combine(localPath, "..", "..", "..", "..", "bin", "al"));
		if (!File.Exists(AssemblyLinker))
		{
			AssemblyLinker = "al";
		}
	}

	[DllImport("kernel32.dll")]
	private static extern uint GetModuleFileName([In] IntPtr hModule, [Out] StringBuilder lpFilename, [In] int nSize);
}
