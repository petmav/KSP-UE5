using System.Collections.Generic;

namespace System.Net.Http.Headers;

public class ProductInfoHeaderValue : ICloneable
{
	public string Comment { get; private set; }

	public ProductHeaderValue Product { get; private set; }

	public ProductInfoHeaderValue(ProductHeaderValue product)
	{
		if (product == null)
		{
			throw new ArgumentNullException();
		}
		Product = product;
	}

	public ProductInfoHeaderValue(string comment)
	{
		Parser.Token.CheckComment(comment);
		Comment = comment;
	}

	public ProductInfoHeaderValue(string productName, string productVersion)
	{
		Product = new ProductHeaderValue(productName, productVersion);
	}

	private ProductInfoHeaderValue()
	{
	}

	object ICloneable.Clone()
	{
		return MemberwiseClone();
	}

	public override bool Equals(object obj)
	{
		if (!(obj is ProductInfoHeaderValue productInfoHeaderValue))
		{
			return false;
		}
		if (Product == null)
		{
			return productInfoHeaderValue.Comment == Comment;
		}
		return Product.Equals(productInfoHeaderValue.Product);
	}

	public override int GetHashCode()
	{
		if (Product == null)
		{
			return Comment.GetHashCode();
		}
		return Product.GetHashCode();
	}

	public static ProductInfoHeaderValue Parse(string input)
	{
		if (TryParse(input, out var parsedValue))
		{
			return parsedValue;
		}
		throw new FormatException(input);
	}

	public static bool TryParse(string input, out ProductInfoHeaderValue parsedValue)
	{
		parsedValue = null;
		Lexer lexer = new Lexer(input);
		if (!TryParseElement(lexer, out parsedValue) || parsedValue == null)
		{
			return false;
		}
		if ((Token.Type)lexer.Scan() != Token.Type.End)
		{
			parsedValue = null;
			return false;
		}
		return true;
	}

	internal static bool TryParse(string input, int minimalCount, out List<ProductInfoHeaderValue> result)
	{
		List<ProductInfoHeaderValue> list = new List<ProductInfoHeaderValue>();
		Lexer lexer = new Lexer(input);
		result = null;
		while (true)
		{
			if (!TryParseElement(lexer, out var parsedValue))
			{
				return false;
			}
			if (parsedValue == null)
			{
				if (list != null && minimalCount <= list.Count)
				{
					result = list;
					return true;
				}
				return false;
			}
			list.Add(parsedValue);
			switch (lexer.PeekChar())
			{
			case 9:
			case 32:
				goto IL_004e;
			case -1:
				if (minimalCount <= list.Count)
				{
					result = list;
					return true;
				}
				break;
			}
			break;
			IL_004e:
			lexer.EatChar();
		}
		return false;
	}

	private static bool TryParseElement(Lexer lexer, out ProductInfoHeaderValue parsedValue)
	{
		parsedValue = null;
		if (lexer.ScanCommentOptional(out var value, out var readToken))
		{
			if (value == null)
			{
				return false;
			}
			parsedValue = new ProductInfoHeaderValue();
			parsedValue.Comment = value;
			return true;
		}
		if ((Token.Type)readToken == Token.Type.End)
		{
			return true;
		}
		if ((Token.Type)readToken != Token.Type.Token)
		{
			return false;
		}
		ProductHeaderValue productHeaderValue = new ProductHeaderValue();
		productHeaderValue.Name = lexer.GetStringValue(readToken);
		int position = lexer.Position;
		readToken = lexer.Scan();
		if ((Token.Type)readToken == Token.Type.SeparatorSlash)
		{
			readToken = lexer.Scan();
			if ((Token.Type)readToken != Token.Type.Token)
			{
				return false;
			}
			productHeaderValue.Version = lexer.GetStringValue(readToken);
		}
		else
		{
			lexer.Position = position;
		}
		parsedValue = new ProductInfoHeaderValue(productHeaderValue);
		return true;
	}

	public override string ToString()
	{
		if (Product == null)
		{
			return Comment;
		}
		return Product.ToString();
	}
}
