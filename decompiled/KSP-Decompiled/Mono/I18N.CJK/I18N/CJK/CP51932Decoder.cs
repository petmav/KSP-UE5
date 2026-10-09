using System;
using I18N.Common;

namespace I18N.CJK;

internal class CP51932Decoder : DbcsEncoding.DbcsDecoder
{
	private int last_count;

	private int last_bytes;

	public CP51932Decoder()
		: base(null)
	{
	}

	public override int GetCharCount(byte[] bytes, int index, int count)
	{
		return GetCharCount(bytes, index, count, refresh: false);
	}

	public override int GetCharCount(byte[] bytes, int index, int count, bool refresh)
	{
		CheckRange(bytes, index, count);
		int num = 0;
		byte[] jisx0208ToUnicode = JISConvert.Convert.jisx0208ToUnicode;
		byte[] jisx0212ToUnicode = JISConvert.Convert.jisx0212ToUnicode;
		int num2 = 0;
		int num3 = 0;
		int num4 = last_count;
		while (count > 0)
		{
			num3 = bytes[index++];
			count--;
			switch (num4)
			{
			case 0:
				if (num3 == 143)
				{
					if (num3 != 0)
					{
						num4 = 0;
						num2++;
					}
					else
					{
						num4 = num3;
					}
					continue;
				}
				if (num3 <= 127)
				{
					num2++;
					continue;
				}
				switch (num3)
				{
				case 142:
					num4 = num3;
					break;
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
				case 250:
				case 251:
				case 252:
				case 253:
				case 254:
					num4 = num3;
					break;
				default:
					num2++;
					break;
				}
				continue;
			case 142:
				if (num3 >= 161 && num3 <= 223)
				{
					num = (num3 - 64) | (num4 + 113 << 8);
					num2++;
				}
				else
				{
					num2++;
				}
				num4 = 0;
				continue;
			case 143:
				num4 = num3;
				continue;
			}
			num = (num4 - 161) * 94;
			num4 = 0;
			if (num3 >= 161 && num3 <= 254)
			{
				num += num3 - 161;
				num *= 2;
				num = jisx0208ToUnicode[num] | (jisx0208ToUnicode[num + 1] << 8);
				if (num == 0)
				{
					num = jisx0212ToUnicode[num] | (jisx0212ToUnicode[num + 1] << 8);
				}
				num2 = ((num == 0) ? (num2 + 1) : (num2 + 1));
			}
			else
			{
				num4 = 0;
				num2++;
			}
		}
		if (refresh && num4 != 0)
		{
			num2++;
		}
		else
		{
			last_count = num4;
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
		int num2 = chars.Length;
		int num3 = last_bytes;
		byte[] jisx0208ToUnicode = JISConvert.Convert.jisx0208ToUnicode;
		byte[] jisx0212ToUnicode = JISConvert.Convert.jisx0212ToUnicode;
		while (byteCount > 0)
		{
			int num4 = bytes[byteIndex++];
			byteCount--;
			int num5;
			switch (num3)
			{
			case 0:
				if (num4 == 143)
				{
					if (num4 != 0)
					{
						num3 = 0;
						if (num >= num2)
						{
							throw Insufficient();
						}
						chars[num++] = '・';
					}
					else
					{
						num3 = num4;
					}
					continue;
				}
				if (num4 <= 127)
				{
					if (num >= num2)
					{
						throw Insufficient();
					}
					chars[num++] = (char)num4;
					continue;
				}
				switch (num4)
				{
				case 142:
					num3 = num4;
					break;
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
				case 250:
				case 251:
				case 252:
				case 253:
				case 254:
					num3 = num4;
					break;
				default:
					if (num >= num2)
					{
						throw Insufficient();
					}
					chars[num++] = '・';
					break;
				}
				continue;
			case 142:
				if (num4 >= 161 && num4 <= 223)
				{
					num5 = (num4 - 64) | (num3 + 113 << 8);
					if (num >= num2)
					{
						throw Insufficient();
					}
					chars[num++] = (char)num5;
				}
				else
				{
					if (num >= num2)
					{
						throw Insufficient();
					}
					chars[num++] = '・';
				}
				num3 = 0;
				continue;
			case 143:
				num3 = num4;
				continue;
			}
			num5 = (num3 - 161) * 94;
			num3 = 0;
			if (num4 >= 161 && num4 <= 254)
			{
				num5 += num4 - 161;
				num5 *= 2;
				num5 = jisx0208ToUnicode[num5] | (jisx0208ToUnicode[num5 + 1] << 8);
				if (num5 == 0)
				{
					num5 = jisx0212ToUnicode[num5] | (jisx0212ToUnicode[num5 + 1] << 8);
				}
				if (num >= num2)
				{
					throw Insufficient();
				}
				if (num5 != 0)
				{
					chars[num++] = (char)num5;
				}
				else
				{
					chars[num++] = '・';
				}
			}
			else
			{
				num3 = 0;
				if (num >= num2)
				{
					throw Insufficient();
				}
				chars[num++] = '・';
			}
		}
		if (refresh && num3 != 0)
		{
			if (num >= num2)
			{
				throw Insufficient();
			}
			chars[num++] = '・';
		}
		else
		{
			last_bytes = num3;
		}
		return num - charIndex;
	}

	private Exception Insufficient()
	{
		throw new ArgumentException(Strings.GetString("Arg_InsufficientSpace"), "chars");
	}
}
