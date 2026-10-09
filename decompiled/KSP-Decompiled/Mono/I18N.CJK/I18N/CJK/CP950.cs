using System;
using System.Text;

namespace I18N.CJK;

[Serializable]
internal class CP950 : DbcsEncoding
{
	private sealed class CP950Decoder : DbcsDecoder
	{
		private int last_byte_count;

		private int last_byte_conv;

		public CP950Decoder(DbcsConvert convert)
			: base(convert)
		{
		}

		public override int GetCharCount(byte[] bytes, int index, int count)
		{
			return GetCharCount(bytes, index, count, refresh: false);
		}

		public override int GetCharCount(byte[] bytes, int index, int count, bool refresh)
		{
			CheckRange(bytes, index, count);
			int num = last_byte_count;
			last_byte_count = 0;
			int num2 = 0;
			while (count-- > 0)
			{
				int num3 = bytes[index++];
				if (num == 0)
				{
					if (num3 > 128)
					{
						switch (num3)
						{
						case 255:
							break;
						default:
							num2++;
							count--;
							continue;
						case 161:
						case 162:
						case 163:
						case 164:
						case 165:
						case 166:
						case 167:
						case 168:
						case 169:
						case 170:
						case 171:
						case 172:
						case 173:
						case 174:
						case 175:
						case 176:
						case 177:
						case 178:
						case 179:
						case 180:
						case 181:
						case 182:
						case 183:
						case 184:
						case 185:
						case 186:
						case 187:
						case 188:
						case 189:
						case 190:
						case 191:
						case 192:
						case 193:
						case 194:
						case 195:
						case 196:
						case 197:
						case 198:
						case 199:
						case 200:
						case 201:
						case 202:
						case 203:
						case 204:
						case 205:
						case 206:
						case 207:
						case 208:
						case 209:
						case 210:
						case 211:
						case 212:
						case 213:
						case 214:
						case 215:
						case 216:
						case 217:
						case 218:
						case 219:
						case 220:
						case 221:
						case 222:
						case 223:
						case 224:
						case 225:
						case 226:
						case 227:
						case 228:
						case 229:
						case 230:
						case 231:
						case 232:
						case 233:
						case 234:
						case 235:
						case 236:
						case 237:
						case 238:
						case 239:
						case 240:
						case 241:
						case 242:
						case 243:
						case 244:
						case 245:
						case 246:
						case 247:
						case 248:
						case 249:
							num = num3;
							continue;
						}
					}
					num2++;
				}
				else
				{
					int num4 = ((num - 161) * 191 + num3 - 64) * 2;
					num2 = ((num4 >= 0 && num4 <= convert.n2u.Length && (ushort)(convert.n2u[num4] + convert.n2u[num4 + 1] * 256) != 0) ? (num2 + 1) : (num2 + 1));
					num = 0;
				}
			}
			if (num != 0)
			{
				if (refresh)
				{
					num2++;
				}
				else
				{
					last_byte_count = num;
				}
			}
			return num2;
		}

		public override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex)
		{
			return GetChars(bytes, byteIndex, byteCount, chars, charIndex, refresh: false);
		}

		public override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex, bool refresh)
		{
			CheckRange(bytes, byteIndex, byteCount, chars, charIndex);
			int num = charIndex;
			int num2 = last_byte_conv;
			last_byte_conv = 0;
			while (byteCount-- > 0)
			{
				int num3 = bytes[byteIndex++];
				if (num2 == 0)
				{
					if (num3 > 128)
					{
						switch (num3)
						{
						case 255:
							break;
						default:
							chars[charIndex++] = '?';
							byteCount--;
							continue;
						case 161:
						case 162:
						case 163:
						case 164:
						case 165:
						case 166:
						case 167:
						case 168:
						case 169:
						case 170:
						case 171:
						case 172:
						case 173:
						case 174:
						case 175:
						case 176:
						case 177:
						case 178:
						case 179:
						case 180:
						case 181:
						case 182:
						case 183:
						case 184:
						case 185:
						case 186:
						case 187:
						case 188:
						case 189:
						case 190:
						case 191:
						case 192:
						case 193:
						case 194:
						case 195:
						case 196:
						case 197:
						case 198:
						case 199:
						case 200:
						case 201:
						case 202:
						case 203:
						case 204:
						case 205:
						case 206:
						case 207:
						case 208:
						case 209:
						case 210:
						case 211:
						case 212:
						case 213:
						case 214:
						case 215:
						case 216:
						case 217:
						case 218:
						case 219:
						case 220:
						case 221:
						case 222:
						case 223:
						case 224:
						case 225:
						case 226:
						case 227:
						case 228:
						case 229:
						case 230:
						case 231:
						case 232:
						case 233:
						case 234:
						case 235:
						case 236:
						case 237:
						case 238:
						case 239:
						case 240:
						case 241:
						case 242:
						case 243:
						case 244:
						case 245:
						case 246:
						case 247:
						case 248:
						case 249:
							num2 = num3;
							continue;
						}
					}
					chars[charIndex++] = (char)num3;
				}
				else
				{
					int num4 = ((num2 - 161) * 191 + num3 - 64) * 2;
					char c = ((num4 >= 0 && num4 <= convert.n2u.Length) ? ((char)(convert.n2u[num4] + convert.n2u[num4 + 1] * 256)) : '\0');
					if (c == '\0')
					{
						chars[charIndex++] = '?';
					}
					else
					{
						chars[charIndex++] = c;
					}
					num2 = 0;
				}
			}
			if (num2 != 0)
			{
				if (refresh)
				{
					chars[charIndex++] = '?';
				}
				else
				{
					last_byte_conv = num2;
				}
			}
			return charIndex - num;
		}
	}

	private const int BIG5_CODE_PAGE = 950;

	public override string BodyName => "big5";

	public override string EncodingName => "Chinese Traditional (Big5)";

	public override string HeaderName => "big5";

	public override string WebName => "big5";

	public CP950()
		: base(950)
	{
	}

	internal override DbcsConvert GetConvert()
	{
		return DbcsConvert.Big5;
	}

	public unsafe override int GetByteCountImpl(char* chars, int count)
	{
		DbcsConvert convert = GetConvert();
		int num = 0;
		int num2 = 0;
		while (count-- > 0)
		{
			char c = *(char*)((byte*)chars + num++ * 2);
			if (c <= '\u0080' || c == 'ÿ')
			{
				num2++;
				continue;
			}
			byte b = convert.u2n[c * 2 + 1];
			byte b2 = convert.u2n[c * 2];
			num2 = ((b != 0 || b2 != 0) ? (num2 + 2) : (num2 + 1));
		}
		return num2;
	}

	public unsafe override int GetBytesImpl(char* chars, int charCount, byte* bytes, int byteCount)
	{
		DbcsConvert convert = GetConvert();
		int charIndex = 0;
		int byteIndex = 0;
		EncoderFallbackBuffer buffer = null;
		int num = byteIndex;
		while (charCount-- > 0)
		{
			char c = *(char*)((byte*)chars + charIndex++ * 2);
			if (c <= '\u0080' || c == 'ÿ')
			{
				bytes[byteIndex++] = (byte)c;
				continue;
			}
			byte b = convert.u2n[c * 2 + 1];
			byte b2 = convert.u2n[c * 2];
			if (b == 0 && b2 == 0)
			{
				HandleFallback(ref buffer, chars, ref charIndex, ref charCount, bytes, ref byteIndex, ref byteCount);
				continue;
			}
			bytes[byteIndex++] = b;
			bytes[byteIndex++] = b2;
		}
		return byteIndex - num;
	}

	public override int GetChars(byte[] bytes, int byteIndex, int byteCount, char[] chars, int charIndex)
	{
		return GetDecoder().GetChars(bytes, byteIndex, byteCount, chars, charIndex);
	}

	public override Decoder GetDecoder()
	{
		return new CP950Decoder(GetConvert());
	}
}
