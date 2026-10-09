public class ModuleConductionMultiplier : PartModule
{
	[KSPField]
	public double modifiedConductionFactor = 1.0;

	[KSPField]
	public double convectionFluxThreshold = double.MaxValue;

	private double originalConduction;

	private double currentConduction;

	private double threshRecip;

	public override void OnAwake()
	{
		originalConduction = base.part.heatConductivity;
	}

	public void Start()
	{
		threshRecip = 1.0 / convectionFluxThreshold;
	}

	private void FixedUpdate()
	{
		if (HighLogic.LoadedSceneIsFlight)
		{
			if (base.part.exposedArea != 0.0 && convectionFluxThreshold != 0.0)
			{
				double num = base.part.thermalConvectionFlux / base.part.exposedArea;
				currentConduction = UtilMath.Lerp(originalConduction, originalConduction * modifiedConductionFactor, (num - convectionFluxThreshold) * threshRecip);
				base.part.heatConductivity = currentConduction;
			}
			else
			{
				base.part.heatConductivity = originalConduction;
			}
		}
	}
}
