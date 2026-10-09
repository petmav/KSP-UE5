namespace System.CodeDom;

[Serializable]
public class CodeChecksumPragma : CodeDirective
{
	private string _fileName;

	public string FileName
	{
		get
		{
			return _fileName ?? string.Empty;
		}
		set
		{
			_fileName = value;
		}
	}

	public Guid ChecksumAlgorithmId { get; set; }

	public byte[] ChecksumData { get; set; }

	public CodeChecksumPragma()
	{
	}

	public CodeChecksumPragma(string fileName, Guid checksumAlgorithmId, byte[] checksumData)
	{
		_fileName = fileName;
		ChecksumAlgorithmId = checksumAlgorithmId;
		ChecksumData = checksumData;
	}
}
