namespace System.CodeDom;

[Serializable]
public class CodeNamespace : CodeObject
{
	private string _name;

	private readonly CodeNamespaceImportCollection _imports = new CodeNamespaceImportCollection();

	private readonly CodeCommentStatementCollection _comments = new CodeCommentStatementCollection();

	private readonly CodeTypeDeclarationCollection _classes = new CodeTypeDeclarationCollection();

	private int _populated;

	private const int ImportsCollection = 1;

	private const int CommentsCollection = 2;

	private const int TypesCollection = 4;

	public CodeTypeDeclarationCollection Types
	{
		get
		{
			if ((_populated & 4) == 0)
			{
				_populated |= 4;
				this.PopulateTypes?.Invoke(this, EventArgs.Empty);
			}
			return _classes;
		}
	}

	public CodeNamespaceImportCollection Imports
	{
		get
		{
			if ((_populated & 1) == 0)
			{
				_populated |= 1;
				this.PopulateImports?.Invoke(this, EventArgs.Empty);
			}
			return _imports;
		}
	}

	public string Name
	{
		get
		{
			return _name ?? string.Empty;
		}
		set
		{
			_name = value;
		}
	}

	public CodeCommentStatementCollection Comments
	{
		get
		{
			if ((_populated & 2) == 0)
			{
				_populated |= 2;
				this.PopulateComments?.Invoke(this, EventArgs.Empty);
			}
			return _comments;
		}
	}

	public event EventHandler PopulateComments;

	public event EventHandler PopulateImports;

	public event EventHandler PopulateTypes;

	public CodeNamespace()
	{
	}

	public CodeNamespace(string name)
	{
		Name = name;
	}
}
