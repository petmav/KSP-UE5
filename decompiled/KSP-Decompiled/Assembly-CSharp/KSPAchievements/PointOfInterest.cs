using FinePrint.Utilities;
using KSP.Localization;
using KSP.UI.Screens;

namespace KSPAchievements
{
	public class PointOfInterest : ProgressNode
	{
		private VesselRef firstVessel;

		private CrewRef firstCrew;

		public string body { get; private set; }

		public string name { get; private set; }

		public string text { get; private set; }

		public bool uplifting { get; private set; }

		public bool launchSite { get; private set; }

		public PointOfInterest(string body, string name, string text, bool uplifting)
			: base("POI" + body + name, startReached: false)
		{
			this.body = body;
			this.name = name;
			this.text = text;
			this.uplifting = uplifting;
			launchSite = false;
			OnDeploy = delegate
			{
				GameEvents.OnPOIRangeEntered.Add(OnAnomalyLoaded);
			};
			OnStow = delegate
			{
				GameEvents.OnPOIRangeEntered.Remove(OnAnomalyLoaded);
			};
			firstVessel = new VesselRef();
			firstCrew = new CrewRef();
		}

		public PointOfInterest(string body, string name, string text, bool uplifting, bool launchSite)
			: this(body, name, text, uplifting)
		{
			this.name = name;
			this.launchSite = launchSite;
		}

		private void OnAnomalyLoaded(CelestialBody anomalyBody, string anomalyName)
		{
			if (anomalyBody.name != body || anomalyName != name || !FlightGlobals.ready)
			{
				return;
			}
			Vessel activeVessel = FlightGlobals.ActiveVessel;
			if (activeVessel == null || !activeVessel.isCommandable || activeVessel.DiscoveryInfo.Level != DiscoveryLevels.Owned)
			{
				return;
			}
			if (!base.IsComplete)
			{
				firstVessel = VesselRef.FromVessel(activeVessel);
				firstCrew = CrewRef.FromVessel(activeVessel);
				Complete();
				CelestialBody celestialBody = null;
				int count = FlightGlobals.Bodies.Count;
				while (count-- > 0)
				{
					if (FlightGlobals.Bodies[count].name == body)
					{
						celestialBody = FlightGlobals.Bodies[count];
						break;
					}
				}
				if (this.launchSite)
				{
					bool flag = false;
					string text = string.Empty;
					string text2 = string.Empty;
					LaunchSite launchSite = null;
					if (PSystemSetup.Instance != null && PSystemSetup.Instance.LaunchSites != null)
					{
						for (int i = 0; i < PSystemSetup.Instance.LaunchSites.Count; i++)
						{
							if (PSystemSetup.Instance.LaunchSites[i].requiresPOIVisit && string.Equals(PSystemSetup.Instance.LaunchSites[i].name, name))
							{
								text = Localizer.Format(PSystemSetup.Instance.LaunchSites[i].launchSiteName);
								flag = true;
								text2 = Localizer.Format(PSystemSetup.Instance.LaunchSites[i].editorFacility.displayDescription());
								launchSite = PSystemSetup.Instance.LaunchSites[i];
								break;
							}
						}
					}
					if (flag)
					{
						ScreenMessages.PostScreenMessage(Localizer.Format("#autoLOC_6006121", text, text2), 10f, persist: false);
						if (MessageSystem.Instance != null)
						{
							MessageSystem.Instance.AddMessage(new MessageSystem.Message(Localizer.Format("#autoLOC_6006122"), Localizer.Format("#autoLOC_6006121", text, text2), MessageSystemButton.MessageButtonColor.BLUE, MessageSystemButton.ButtonIcons.MESSAGE));
						}
						if (launchSite != null)
						{
							GameEvents.LaunchSiteFound.Fire(launchSite);
						}
					}
					else if (MessageSystem.Instance != null)
					{
						MessageSystem.Instance.AddMessage(new MessageSystem.Message(Localizer.Format("#autoLOC_8003616"), Localizer.Format("#autoLOC_296488", this.text, celestialBody.displayName), MessageSystemButton.MessageButtonColor.BLUE, MessageSystemButton.ButtonIcons.MESSAGE));
					}
				}
				else
				{
					if (uplifting)
					{
						int pQSCitySeed = PQSCity.GetPQSCitySeed(anomalyName, anomalyBody.bodyName);
						string text3 = Localizer.Format("#autoLOC_296488", this.text, celestialBody.displayName);
						text3 += GetUpliftText(pQSCitySeed);
						AwardProgressRandomTech(text3, pQSCitySeed);
					}
					else
					{
						AwardProgressStandard(Localizer.Format("#autoLOC_296494", this.text, celestialBody.displayName), ProgressType.POINTOFINTEREST, celestialBody);
					}
					if (HighLogic.CurrentGame.Mode != Game.Modes.CAREER && MessageSystem.Instance != null)
					{
						MessageSystem.Instance.AddMessage(new MessageSystem.Message(Localizer.Format("#autoLOC_8003616"), Localizer.Format("#autoLOC_296488", this.text, celestialBody.displayName), MessageSystemButton.MessageButtonColor.BLUE, MessageSystemButton.ButtonIcons.MESSAGE));
					}
				}
				AnalyticsUtil.LogPointOfInterestReached(this, celestialBody);
			}
			Achieve();
		}

		protected override void OnLoad(ConfigNode node)
		{
			if (node.HasNode("vessel"))
			{
				firstVessel.Load(node.GetNode("vessel"));
			}
			if (node.HasNode("crew"))
			{
				firstCrew.Load(node.GetNode("crew"));
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			firstVessel.Save(node.AddNode("vessel"));
			if (firstCrew.HasAny)
			{
				firstCrew.Save(node.AddNode("crew"));
			}
		}

		protected string GetUpliftText(int seed)
		{
			switch (new KSPRandom(seed).Next(10))
			{
			default:
				return Localizer.Format("#autoLOC_296540");
			case 0:
				return Localizer.Format("#autoLOC_296522");
			case 1:
				return Localizer.Format("#autoLOC_296524");
			case 2:
				return Localizer.Format("#autoLOC_296526");
			case 3:
				return Localizer.Format("#autoLOC_296528");
			case 4:
				return Localizer.Format("#autoLOC_296530");
			case 5:
				return Localizer.Format("#autoLOC_296532");
			case 6:
				return Localizer.Format("#autoLOC_296534");
			case 7:
				return Localizer.Format("#autoLOC_296536");
			case 8:
				return Localizer.Format("#autoLOC_296538");
			}
		}
	}
}
