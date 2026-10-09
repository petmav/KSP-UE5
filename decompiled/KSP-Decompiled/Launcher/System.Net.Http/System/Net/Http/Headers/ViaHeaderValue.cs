using System.Collections.Generic;

namespace System.Net.Http.Headers;

public class ViaHeaderValue : ICloneable
{
	public string Comment { get; private set; }

	public string ProtocolName { get; private set; }

	public string ProtocolVersion { get; private set; }

	public string ReceivedBy { get; private set; }

	public ViaHeaderValue(string protocolVersion, string receivedBy)
	{
		Parser.Token.Check(protocolVersion);
		Parser.Uri.Check(receivedBy);
		ProtocolVersion = protocolVersion;
		ReceivedBy = receivedBy;
	}

	public ViaHeaderValue(string protocolVersion, string receivedBy, string protocolName)
		: this(protocolVersion, receivedBy)
	{
		if (!string.IsNullOrEmpty(protocolName))
		{
			Parser.Token.Check(protocolName);
			ProtocolName = protocolName;
		}
	}

	public ViaHeaderValue(string protocolVersion, string receivedBy, string protocolName, string comment)
		: this(protocolVersion, receivedBy, protocolName)
	{
		if (!string.IsNullOrEmpty(comment))
		{
			Parser.Token.CheckComment(comment);
			Comment = comment;
		}
	}

	private ViaHeaderValue()
	{
	}

	object ICloneable.Clone()
	{
		return MemberwiseClone();
	}

	public override bool Equals(object obj)
	{
		if (!(obj is ViaHeaderValue viaHeaderValue))
		{
			return false;
		}
		if (string.Equals(viaHeaderValue.Comment, Comment, StringComparison.Ordinal) && string.Equals(viaHeaderValue.ProtocolName, ProtocolName, StringComparison.OrdinalIgnoreCase) && string.Equals(viaHeaderValue.ProtocolVersion, ProtocolVersion, StringComparison.OrdinalIgnoreCase))
		{
			return string.Equals(viaHeaderValue.ReceivedBy, ReceivedBy, StringComparison.OrdinalIgnoreCase);
		}
		return false;
	}

	public override int GetHashCode()
	{
		int hashCode = ProtocolVersion.ToLowerInvariant().GetHashCode();
		hashCode ^= ReceivedBy.ToLowerInvariant().GetHashCode();
		if (!string.IsNullOrEmpty(ProtocolName))
		{
			hashCode ^= ProtocolName.ToLowerInvariant().GetHashCode();
		}
		if (!string.IsNullOrEmpty(Comment))
		{
			hashCode ^= Comment.GetHashCode();
		}
		return hashCode;
	}

	public static ViaHeaderValue Parse(string input)
	{
		if (TryParse(input, out var parsedValue))
		{
			return parsedValue;
		}
		throw new FormatException(input);
	}

	public static bool TryParse(string input, out ViaHeaderValue parsedValue)
	{
		if (TryParseElement(new Lexer(input), out parsedValue, out var t) && (Token.Type)t == Token.Type.End)
		{
			return true;
		}
		parsedValue = null;
		return false;
	}

	internal static bool TryParse(string input, int minimalCount, out List<ViaHeaderValue> result)
	{
		return CollectionParser.TryParse(input, minimalCount, (ElementTryParser<ViaHeaderValue>)TryParseElement, out result);
	}

	private static bool TryParseElement(Lexer lexer, out ViaHeaderValue parsedValue, out Token t)
	{
		parsedValue = null;
		t = lexer.Scan();
		if ((Token.Type)t != Token.Type.Token)
		{
			return false;
		}
		Token token = lexer.Scan();
		ViaHeaderValue viaHeaderValue = new ViaHeaderValue();
		if ((Token.Type)token == Token.Type.SeparatorSlash)
		{
			token = lexer.Scan();
			if ((Token.Type)token != Token.Type.Token)
			{
				return false;
			}
			viaHeaderValue.ProtocolName = lexer.GetStringValue(t);
			viaHeaderValue.ProtocolVersion = lexer.GetStringValue(token);
			token = lexer.Scan();
		}
		else
		{
			viaHeaderValue.ProtocolVersion = lexer.GetStringValue(t);
		}
		if ((Token.Type)token != Token.Type.Token)
		{
			return false;
		}
		if (lexer.PeekChar() == 58)
		{
			lexer.EatChar();
			t = lexer.Scan();
			if ((Token.Type)t != Token.Type.Token)
			{
				return false;
			}
		}
		else
		{
			t = token;
		}
		viaHeaderValue.ReceivedBy = lexer.GetStringValue(token, t);
		if (lexer.ScanCommentOptional(out var value, out t))
		{
			t = lexer.Scan();
		}
		viaHeaderValue.Comment = value;
		parsedValue = viaHeaderValue;
		return true;
	}

	public override string ToString()
	{
		string text = ((ProtocolName != null) ? (ProtocolName + "/" + ProtocolVersion + " " + ReceivedBy) : (ProtocolVersion + " " + ReceivedBy));
		if (Comment == null)
		{
			return text;
		}
		return text + " " + Comment;
	}
}
