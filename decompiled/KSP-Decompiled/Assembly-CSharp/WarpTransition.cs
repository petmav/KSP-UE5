internal class WarpTransition
{
	public int Index { get; private set; }

	public double Rate { get; private set; }

	public double UTTo1Times { get; set; }

	public double UTToRateDown { get; set; }

	public double UTToRateUp { get; set; }

	internal WarpTransition(int Index, double Rate)
	{
		this.Index = Index;
		this.Rate = Rate;
		double uTToRateDown = 0.0;
		UTToRateUp = 0.0;
		double uTTo1Times = 0.0;
		UTToRateDown = uTToRateDown;
		UTTo1Times = uTTo1Times;
	}
}
