using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Text;

namespace Mono.Net.Dns;

internal class DnsResponse : DnsPacket
{
	private static readonly ReadOnlyCollection<DnsResourceRecord> EmptyRR = new ReadOnlyCollection<DnsResourceRecord>(new DnsResourceRecord[0]);

	private static readonly ReadOnlyCollection<DnsQuestion> EmptyQS = new ReadOnlyCollection<DnsQuestion>(new DnsQuestion[0]);

	private ReadOnlyCollection<DnsQuestion> question;

	private ReadOnlyCollection<DnsResourceRecord> answer;

	private ReadOnlyCollection<DnsResourceRecord> authority;

	private ReadOnlyCollection<DnsResourceRecord> additional;

	private int offset = 12;

	public DnsResponse(byte[] buffer, int length)
		: base(buffer, length)
	{
	}

	public void Reset()
	{
		question = null;
		answer = null;
		authority = null;
		additional = null;
		for (int i = 0; i < packet.Length; i++)
		{
			packet[i] = 0;
		}
	}

	private ReadOnlyCollection<DnsResourceRecord> GetRRs(int count)
	{
		if (count <= 0)
		{
			return EmptyRR;
		}
		List<DnsResourceRecord> list = new List<DnsResourceRecord>(count);
		for (int i = 0; i < count; i++)
		{
			list.Add(DnsResourceRecord.CreateFromBuffer(this, position, ref offset));
		}
		return list.AsReadOnly();
	}

	private ReadOnlyCollection<DnsQuestion> GetQuestions(int count)
	{
		if (count <= 0)
		{
			return EmptyQS;
		}
		List<DnsQuestion> list = new List<DnsQuestion>(count);
		for (int i = 0; i < count; i++)
		{
			DnsQuestion dnsQuestion = new DnsQuestion();
			offset = dnsQuestion.Init(this, offset);
			list.Add(dnsQuestion);
		}
		return list.AsReadOnly();
	}

	public ReadOnlyCollection<DnsQuestion> GetQuestions()
	{
		if (question == null)
		{
			question = GetQuestions(base.Header.QuestionCount);
		}
		return question;
	}

	public ReadOnlyCollection<DnsResourceRecord> GetAnswers()
	{
		if (answer == null)
		{
			GetQuestions();
			answer = GetRRs(base.Header.AnswerCount);
		}
		return answer;
	}

	public ReadOnlyCollection<DnsResourceRecord> GetAuthority()
	{
		if (authority == null)
		{
			GetQuestions();
			GetAnswers();
			authority = GetRRs(base.Header.AuthorityCount);
		}
		return authority;
	}

	public ReadOnlyCollection<DnsResourceRecord> GetAdditional()
	{
		if (additional == null)
		{
			GetQuestions();
			GetAnswers();
			GetAuthority();
			additional = GetRRs(base.Header.AdditionalCount);
		}
		return additional;
	}

	public override string ToString()
	{
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append(base.Header);
		stringBuilder.Append("Question:\r\n");
		foreach (DnsQuestion question in GetQuestions())
		{
			stringBuilder.AppendFormat("\t{0}\r\n", question);
		}
		stringBuilder.Append("Answer(s):\r\n");
		foreach (DnsResourceRecord answer in GetAnswers())
		{
			stringBuilder.AppendFormat("\t{0}\r\n", answer);
		}
		stringBuilder.Append("Authority:\r\n");
		foreach (DnsResourceRecord item in GetAuthority())
		{
			stringBuilder.AppendFormat("\t{0}\r\n", item);
		}
		stringBuilder.Append("Additional:\r\n");
		foreach (DnsResourceRecord item2 in GetAdditional())
		{
			stringBuilder.AppendFormat("\t{0}\r\n", item2);
		}
		return stringBuilder.ToString();
	}
}
