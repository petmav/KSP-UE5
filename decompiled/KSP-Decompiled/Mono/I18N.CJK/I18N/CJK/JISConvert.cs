namespace I18N.CJK;

internal sealed class JISConvert
{
	private const int JISX0208_To_Unicode = 1;

	private const int JISX0212_To_Unicode = 2;

	private const int CJK_To_JIS = 3;

	private const int Greek_To_JIS = 4;

	private const int Extra_To_JIS = 5;

	public byte[] jisx0208ToUnicode;

	public byte[] jisx0212ToUnicode;

	public byte[] cjkToJis;

	public byte[] greekToJis;

	public byte[] extraToJis;

	private static JISConvert convert;

	private static readonly object lockobj = new object();

	public static JISConvert Convert
	{
		get
		{
			lock (lockobj)
			{
				if (convert != null)
				{
					return convert;
				}
				convert = new JISConvert();
				return convert;
			}
		}
	}

	private JISConvert()
	{
		CodeTable codeTable = new CodeTable("jis.table");
		jisx0208ToUnicode = codeTable.GetSection(1);
		jisx0212ToUnicode = codeTable.GetSection(2);
		cjkToJis = codeTable.GetSection(3);
		greekToJis = codeTable.GetSection(4);
		extraToJis = codeTable.GetSection(5);
		codeTable.Dispose();
	}
}
