using System.Runtime.InteropServices;

namespace Lingoona
{
	internal class NativeMethods
	{
		private const string nativeLibrary = "lingoona_grammar_kerbal";

		[DllImport("lingoona_grammar_kerbal", CallingConvention = CallingConvention.Cdecl)]
		public static extern void initialize();

		[DllImport("lingoona_grammar_kerbal", CallingConvention = CallingConvention.Cdecl)]
		public static extern bool isAvailable();

		[DllImport("lingoona_grammar_kerbal", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
		public static extern void setLanguage([MarshalAs(UnmanagedType.LPWStr)] string language);

		[DllImport("lingoona_grammar_kerbal", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
		[return: MarshalAs(UnmanagedType.LPWStr)]
		public static extern string getLanguage();

		[DllImport("lingoona_grammar_kerbal", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
		[return: MarshalAs(UnmanagedType.LPWStr)]
		public static extern string useGrammar([MarshalAs(UnmanagedType.LPWStr)] string template, int length, [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPWStr)] string[] parameters, int pcount);

		[DllImport("lingoona_grammar_kerbal", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
		[return: MarshalAs(UnmanagedType.LPWStr)]
		public static extern string generateName([MarshalAs(UnmanagedType.LPWStr)] string noun, int length, [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPWStr)] string[] components, int ccount);

		[DllImport("lingoona_grammar_kerbal", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
		public static extern void setVariable([MarshalAs(UnmanagedType.LPWStr)] string name, [MarshalAs(UnmanagedType.LPWStr)] string value);

		[DllImport("lingoona_grammar_kerbal", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
		public static extern void clearVariable([MarshalAs(UnmanagedType.LPWStr)] string name);

		[DllImport("lingoona_grammar_kerbal", CallingConvention = CallingConvention.Cdecl)]
		public static extern void clearVariables();
	}
}
