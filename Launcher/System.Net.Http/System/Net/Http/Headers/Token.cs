namespace System.Net.Http.Headers;

internal struct Token
{
	public enum Type
	{
		Error,
		End,
		Token,
		QuotedString,
		SeparatorEqual,
		SeparatorSemicolon,
		SeparatorSlash,
		SeparatorDash,
		SeparatorComma,
		OpenParens
	}

	public static readonly Token Empty = new Token(Type.Token, 0, 0);

	private readonly Type type;

	public int StartPosition { get; private set; }

	public int EndPosition { get; private set; }

	public Type Kind => type;

	public Token(Type type, int startPosition, int endPosition)
	{
		this = default(Token);
		this.type = type;
		StartPosition = startPosition;
		EndPosition = endPosition;
	}

	public static implicit operator Type(Token token)
	{
		return token.type;
	}

	public override string ToString()
	{
		return type.ToString();
	}
}
