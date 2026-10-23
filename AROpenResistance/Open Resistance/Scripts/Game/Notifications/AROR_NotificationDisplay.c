//! Client-side drawing of notifications and hints (driven by AROR_NotificationManagerComponent).
//! Headlines (full-width strip) and notifications (dispatch card, top right) share one queue and show one at a time;
//! headlines jump ahead of queued notifications. Hints (bottom pill) bypass the queue: a new hint replaces the current one.
//! The layout (AROR_Notifications.layout) only declares the widgets; positions, sizes and styling are set here.
class AROR_NotificationDisplay : Managed
{
	//! Above the HUD and the map
	protected static const int ROOT_Z_ORDER = 1000;
	protected static const float FADE_MS = 250;

	// Headline strip, in reference pixels
	protected static const float HEADLINE_SCREEN_Y = 0.22;	//!< Vertical centre as a fraction of screen height
	protected static const float HEADLINE_HEIGHT = 56;
	protected static const float HEADLINE_ICON_SIZE = 30;
	protected static const float HEADLINE_GAP = 14;
	protected static const float HEADLINE_TIMER_FRACTION = 0.64;
	protected static const int HEADLINE_FONT_SIZE = 26;

	// Dispatch card
	protected static const float CARD_MARGIN = 20;
	protected static const float CARD_TOP = 80;
	protected static const float CARD_HEIGHT = 58;
	protected static const float CARD_ICON_BLOCK = 56;
	protected static const float CARD_ICON_SIZE = 28;
	protected static const float CARD_PADDING = 12;
	protected static const float CARD_MIN_WIDTH = 300;
	protected static const float CARD_MAX_WIDTH = 480;	//!< Longer text wraps
	protected static const float CARD_MAX_LINES = 3;
	protected static const float CARD_KICKER_TOP = 9;
	protected static const float CARD_TEXT_TOP = 25;
	protected static const int CARD_KICKER_FONT_SIZE = 12;
	protected static const int CARD_TEXT_FONT_SIZE = 18;

	// Hint pill
	protected static const float HINT_BOTTOM = 140;
	protected static const float HINT_HEIGHT = 36;
	protected static const float HINT_PADDING = 10;
	protected static const float HINT_BORDER = 2;
	protected static const float HINT_ICON_SIZE = 20;
	protected static const float HINT_GAP = 10;
	protected static const int HINT_FONT_SIZE = 15;
	protected static const int HINT_BADGE_FONT_SIZE = 15;

	protected Widget m_wRoot;

	protected Widget m_wHeadline;
	protected ImageWidget m_wHeadlineBand;
	protected ImageWidget m_wHeadlineLineTop;
	protected ImageWidget m_wHeadlineLineBottom;
	protected ImageWidget m_wHeadlineIcon;
	protected RichTextWidget m_wHeadlineText;
	protected ImageWidget m_wHeadlineTimer;

	protected Widget m_wCard;
	protected ImageWidget m_wCardBackground;
	protected ImageWidget m_wCardIconBlock;
	protected ImageWidget m_wCardIcon;
	protected RichTextWidget m_wCardKicker;
	protected RichTextWidget m_wCardText;
	protected ImageWidget m_wCardTimer;

	protected Widget m_wHint;
	protected ImageWidget m_wHintBackground;
	protected ref array<ImageWidget> m_aHintBorders = {};
	protected ImageWidget m_wHintIcon;
	protected ImageWidget m_wHintBadge;			//!< "i" badge drawn when no hint icon texture is set
	protected RichTextWidget m_wHintBadgeText;
	protected RichTextWidget m_wHintText;

	protected float m_fHeadlineDurationMs;
	protected float m_fNotificationDurationMs;
	protected float m_fHintDurationMs;

	protected ref array<ref AROR_Notification> m_aQueue = {};
	protected ref AROR_Notification m_Current;
	protected int m_iCurrentStartMs;
	protected bool m_bCurrentHasIcon;
	protected int m_iCurrentFrames;

	//! Text widgets report their new size only after a layout pass, so the card is sized a couple of frames after SetText
	protected static const int MEASURE_DELAY_FRAMES = 2;
	protected bool m_bCardSized;
	protected float m_fCardWidth;
	protected float m_fCardHeight;
	protected float m_fCardTextHeight;

	protected bool m_bHintActive;
	protected bool m_bHintHasIcon;
	protected int m_iHintStartMs;

	protected bool m_bUpdating;

	//------------------------------------------------------------------------------------------------
	void AROR_NotificationDisplay(ResourceName layout, float headlineSeconds, float notificationSeconds, float hintSeconds)
	{
		m_fHeadlineDurationMs = headlineSeconds * 1000;
		m_fNotificationDurationMs = notificationSeconds * 1000;
		m_fHintDurationMs = hintSeconds * 1000;

		m_wRoot = GetGame().GetWorkspace().CreateWidgets(layout);
		if (!m_wRoot)
		{
			Print("[AROR] Notifications layout could not be created", LogLevel.ERROR);
			return;
		}

		m_wHeadline = m_wRoot.FindAnyWidget("Headline");
		m_wHeadlineBand = ImageWidget.Cast(m_wRoot.FindAnyWidget("HeadlineBand"));
		m_wHeadlineLineTop = ImageWidget.Cast(m_wRoot.FindAnyWidget("HeadlineLineTop"));
		m_wHeadlineLineBottom = ImageWidget.Cast(m_wRoot.FindAnyWidget("HeadlineLineBottom"));
		m_wHeadlineIcon = ImageWidget.Cast(m_wRoot.FindAnyWidget("HeadlineIcon"));
		m_wHeadlineText = RichTextWidget.Cast(m_wRoot.FindAnyWidget("HeadlineText"));
		m_wHeadlineTimer = ImageWidget.Cast(m_wRoot.FindAnyWidget("HeadlineTimer"));

		m_wCard = m_wRoot.FindAnyWidget("Card");
		m_wCardBackground = ImageWidget.Cast(m_wRoot.FindAnyWidget("CardBackground"));
		m_wCardIconBlock = ImageWidget.Cast(m_wRoot.FindAnyWidget("CardIconBlock"));
		m_wCardIcon = ImageWidget.Cast(m_wRoot.FindAnyWidget("CardIcon"));
		m_wCardKicker = RichTextWidget.Cast(m_wRoot.FindAnyWidget("CardKicker"));
		m_wCardText = RichTextWidget.Cast(m_wRoot.FindAnyWidget("CardText"));
		m_wCardTimer = ImageWidget.Cast(m_wRoot.FindAnyWidget("CardTimer"));

		m_wHint = m_wRoot.FindAnyWidget("Hint");
		m_wHintBackground = ImageWidget.Cast(m_wRoot.FindAnyWidget("HintBackground"));
		m_wHintIcon = ImageWidget.Cast(m_wRoot.FindAnyWidget("HintIcon"));
		m_wHintBadge = ImageWidget.Cast(m_wRoot.FindAnyWidget("HintBadge"));
		m_wHintBadgeText = RichTextWidget.Cast(m_wRoot.FindAnyWidget("HintBadgeText"));
		m_wHintText = RichTextWidget.Cast(m_wRoot.FindAnyWidget("HintText"));

		array<string> borderNames = {"HintBorderTop", "HintBorderBottom", "HintBorderLeft", "HintBorderRight"};
		foreach (string borderName : borderNames)
		{
			ImageWidget border = ImageWidget.Cast(m_wRoot.FindAnyWidget(borderName));
			if (border)
				m_aHintBorders.Insert(border);
		}

		if (!m_wHeadline || !m_wHeadlineText || !m_wCard || !m_wCardText || !m_wHint || !m_wHintText)
		{
			Print("[AROR] Notifications layout is missing widgets (Headline, Card, Hint groups)", LogLevel.ERROR);
			m_wRoot.RemoveFromHierarchy();
			m_wRoot = null;
			return;
		}

		ApplyLayout();
		ApplyStyle();

		m_wHeadline.SetVisible(false);
		m_wCard.SetVisible(false);
		m_wHint.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	void ~AROR_NotificationDisplay()
	{
		GetGame().GetCallqueue().Remove(Update);

		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();
	}

	//------------------------------------------------------------------------------------------------
	//! Queue a notification; headlines go ahead of queued notifications but behind queued headlines
	void Enqueue(notnull AROR_Notification notification)
	{
		if (!m_wRoot)
			return;

		int index = m_aQueue.Count();
		if (notification.GetNotificationType() == AROR_ENotificationType.HEADLINE)
		{
			index = 0;
			while (index < m_aQueue.Count() && m_aQueue[index].GetNotificationType() == AROR_ENotificationType.HEADLINE)
				index++;
		}

		m_aQueue.InsertAt(notification, index);

		if (!m_Current)
			ShowNext();
	}

	//------------------------------------------------------------------------------------------------
	//! Show a hint now, replacing any current one
	void ShowHint(string text, notnull Color color, ResourceName icon, string sound)
	{
		if (!m_wRoot)
			return;

		m_wHintText.SetText(text);

		// A texture icon when one is set, otherwise the "i" badge
		m_bHintHasIcon = SetIcon(m_wHintIcon, icon);
		SetColor(m_wHintIcon, color);
		SetColor(m_wHintBadge, color);
		SetVisible(m_wHintBadge, !m_bHintHasIcon);
		SetVisible(m_wHintBadgeText, !m_bHintHasIcon);

		foreach (ImageWidget border : m_aHintBorders)
		{
			border.SetColor(color);
		}

		// Dark background tinted towards the hint colour
		SetColor(m_wHintBackground, new Color(0.04 + color.R() * 0.12, 0.04 + color.G() * 0.12, 0.03 + color.B() * 0.12, 0.88));

		m_iHintStartMs = System.GetTickCount();
		m_bHintActive = true;
		m_wHint.SetOpacity(0);
		m_wHint.SetVisible(true);

		PlaySound(sound);
		StartUpdating();
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowNext()
	{
		m_Current = null;
		m_wHeadline.SetVisible(false);
		m_wCard.SetVisible(false);

		if (m_aQueue.IsEmpty())
			return;

		m_Current = m_aQueue[0];
		m_aQueue.RemoveOrdered(0);

		Color color = m_Current.GetColor();
		if (m_Current.GetNotificationType() == AROR_ENotificationType.HEADLINE)
		{
			// ToUpper modifies the string in place
			string text = m_Current.GetText();
			text.ToUpper();
			m_wHeadlineText.SetText(text);

			m_bCurrentHasIcon = SetIcon(m_wHeadlineIcon, m_Current.GetIcon());
			SetColor(m_wHeadlineIcon, color);
			SetColor(m_wHeadlineLineTop, color);
			SetColor(m_wHeadlineLineBottom, color);
			SetColor(m_wHeadlineTimer, color);

			m_wHeadline.SetOpacity(0);
			m_wHeadline.SetVisible(true);
		}
		else
		{
			string kicker = m_Current.GetKicker();
			kicker.ToUpper();
			if (m_wCardKicker)
				m_wCardKicker.SetText(kicker);

			// Single line first, to measure; SizeCard switches to wrapping if it is too long
			m_wCardText.SetTextWrapping(false);
			FrameSlot.SetSizeToContent(m_wCardText, true);
			m_wCardText.SetText(m_Current.GetText());
			m_bCardSized = false;

			// Icon drawn white on a block filled with the accent colour
			m_bCurrentHasIcon = SetIcon(m_wCardIcon, m_Current.GetIcon());
			SetColor(m_wCardIconBlock, color);
			SetColor(m_wCardTimer, color);

			m_wCard.SetOpacity(0);
			m_wCard.SetVisible(true);
		}

		m_iCurrentStartMs = System.GetTickCount();
		m_iCurrentFrames = 0;
		PlaySound(m_Current.GetSound());
		StartUpdating();
	}

	//------------------------------------------------------------------------------------------------
	protected void StartUpdating()
	{
		if (m_bUpdating)
			return;

		m_bUpdating = true;
		GetGame().GetCallqueue().CallLater(Update, 0, true);
	}

	//------------------------------------------------------------------------------------------------
	//! Every frame while something is on screen: fades, timer bars, and sizes that depend on the text
	protected void Update()
	{
		int now = System.GetTickCount();

		if (m_Current)
			UpdateCurrent(now);

		if (m_bHintActive)
			UpdateHint(now);

		if (!m_Current && !m_bHintActive)
		{
			m_bUpdating = false;
			GetGame().GetCallqueue().Remove(Update);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateCurrent(int now)
	{
		bool isHeadline = m_Current.GetNotificationType() == AROR_ENotificationType.HEADLINE;
		float duration = m_fNotificationDurationMs;
		if (isHeadline)
			duration = m_fHeadlineDurationMs;

		float elapsed = now - m_iCurrentStartMs;
		if (elapsed >= duration)
		{
			ShowNext();
			return;
		}

		m_iCurrentFrames++;
		float opacity = GetFadeOpacity(elapsed, duration);
		float remaining = 1 - elapsed / duration;

		if (isHeadline)
		{
			m_wHeadline.SetOpacity(opacity);
			LayoutHeadline(remaining);
			return;
		}

		if (!m_bCardSized && m_iCurrentFrames > MEASURE_DELAY_FRAMES)
			SizeCard();

		// Stay invisible until sized, so the previous message's size never shows
		if (!m_bCardSized)
		{
			m_wCard.SetOpacity(0);
			return;
		}

		m_wCard.SetOpacity(opacity);
		LayoutCard(remaining);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateHint(int now)
	{
		float elapsed = now - m_iHintStartMs;
		if (elapsed >= m_fHintDurationMs)
		{
			m_bHintActive = false;
			m_wHint.SetVisible(false);
			return;
		}

		m_wHint.SetOpacity(GetFadeOpacity(elapsed, m_fHintDurationMs));
		LayoutHint();
	}

	//------------------------------------------------------------------------------------------------
	protected static float GetFadeOpacity(float elapsed, float duration)
	{
		float fadeIn = elapsed / FADE_MS;
		float fadeOut = (duration - elapsed) / FADE_MS;
		return Math.Clamp(Math.Min(fadeIn, fadeOut), 0, 1);
	}

	//------------------------------------------------------------------------------------------------
	//! Full-width band; icon and text centred together
	protected void LayoutHeadline(float remaining)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		float width = workspace.DPIUnscale(workspace.GetWidth());

		FrameSlot.SetSize(m_wHeadline, width, HEADLINE_HEIGHT);
		Place(m_wHeadlineBand, 0, 0, width, HEADLINE_HEIGHT);
		Place(m_wHeadlineLineTop, 0, 0, width, 1);
		Place(m_wHeadlineLineBottom, 0, HEADLINE_HEIGHT - 1, width, 1);

		float textWidth, textHeight;
		MeasureText(m_wHeadlineText, HEADLINE_FONT_SIZE, textWidth, textHeight);

		float iconSpace;
		if (m_bCurrentHasIcon)
			iconSpace = HEADLINE_ICON_SIZE + HEADLINE_GAP;

		float left = (width - iconSpace - textWidth) * 0.5;
		Place(m_wHeadlineIcon, left, (HEADLINE_HEIGHT - HEADLINE_ICON_SIZE) * 0.5, HEADLINE_ICON_SIZE, HEADLINE_ICON_SIZE);
		FrameSlot.SetPos(m_wHeadlineText, left + iconSpace, (HEADLINE_HEIGHT - textHeight) * 0.5);

		// Drains towards the centre from both ends
		float timerWidth = width * HEADLINE_TIMER_FRACTION * remaining;
		Place(m_wHeadlineTimer, (width - timerWidth) * 0.5, HEADLINE_HEIGHT - 3, timerWidth, 2);
	}

	//------------------------------------------------------------------------------------------------
	//! Once per card: width follows the text up to CARD_MAX_WIDTH, then the text wraps (up to CARD_MAX_LINES lines)
	protected void SizeCard()
	{
		float textWidth, lineHeight, kickerWidth, kickerHeight;
		MeasureText(m_wCardText, CARD_TEXT_FONT_SIZE, textWidth, lineHeight);
		if (m_wCardKicker)
			MeasureText(m_wCardKicker, CARD_KICKER_FONT_SIZE, kickerWidth, kickerHeight);

		float maxTextWidth = CARD_MAX_WIDTH - CARD_ICON_BLOCK - CARD_PADDING * 2;
		float contentWidth = Math.Max(textWidth, kickerWidth);
		float lines = 1;

		if (textWidth > maxTextWidth)
		{
			// Word wrapping breaks lines early, so allow some slack when counting them
			lines = Math.Min(Math.Ceil(textWidth * 1.15 / maxTextWidth), CARD_MAX_LINES);
			contentWidth = maxTextWidth;

			FrameSlot.SetSizeToContent(m_wCardText, false);
			m_wCardText.SetTextWrapping(true);
			FrameSlot.SetSize(m_wCardText, maxTextWidth, lines * lineHeight);
		}

		m_fCardTextHeight = lines * lineHeight;
		m_fCardWidth = Math.Clamp(CARD_ICON_BLOCK + CARD_PADDING + contentWidth + CARD_PADDING, CARD_MIN_WIDTH, CARD_MAX_WIDTH);
		m_fCardHeight = Math.Max(CARD_HEIGHT, CARD_TEXT_TOP + m_fCardTextHeight + CARD_PADDING);
		m_bCardSized = true;
	}

	//------------------------------------------------------------------------------------------------
	//! Coloured icon block on the left, kicker above the text
	protected void LayoutCard(float remaining)
	{
		float width = m_fCardWidth;
		float height = m_fCardHeight;
		float textLeft = CARD_ICON_BLOCK + CARD_PADDING;

		FrameSlot.SetSize(m_wCard, width, height);
		Place(m_wCardBackground, 0, 0, width, height);
		Place(m_wCardIconBlock, 0, 0, CARD_ICON_BLOCK, height);
		Place(m_wCardIcon, (CARD_ICON_BLOCK - CARD_ICON_SIZE) * 0.5, (height - CARD_ICON_SIZE) * 0.5, CARD_ICON_SIZE, CARD_ICON_SIZE);

		if (m_wCardKicker)
			FrameSlot.SetPos(m_wCardKicker, textLeft, CARD_KICKER_TOP);

		FrameSlot.SetPos(m_wCardText, textLeft, CARD_TEXT_TOP);
		Place(m_wCardTimer, CARD_ICON_BLOCK, height - 2, (width - CARD_ICON_BLOCK) * remaining, 2);
	}

	//------------------------------------------------------------------------------------------------
	//! Pill at the bottom centre, outlined in the hint colour, with the icon (or "i" badge) before the text
	protected void LayoutHint()
	{
		float textWidth, textHeight;
		MeasureText(m_wHintText, HINT_FONT_SIZE, textWidth, textHeight);

		float iconSpace = HINT_ICON_SIZE + HINT_GAP;
		float width = HINT_PADDING + iconSpace + textWidth + HINT_PADDING + HINT_GAP * 0.5;

		FrameSlot.SetSize(m_wHint, width, HINT_HEIGHT);
		Place(m_wHintBackground, 0, 0, width, HINT_HEIGHT);

		if (m_aHintBorders.Count() == 4)
		{
			Place(m_aHintBorders[0], 0, 0, width, HINT_BORDER);
			Place(m_aHintBorders[1], 0, HINT_HEIGHT - HINT_BORDER, width, HINT_BORDER);
			Place(m_aHintBorders[2], 0, 0, HINT_BORDER, HINT_HEIGHT);
			Place(m_aHintBorders[3], width - HINT_BORDER, 0, HINT_BORDER, HINT_HEIGHT);
		}

		float iconTop = (HINT_HEIGHT - HINT_ICON_SIZE) * 0.5;
		Place(m_wHintIcon, HINT_PADDING, iconTop, HINT_ICON_SIZE, HINT_ICON_SIZE);
		Place(m_wHintBadge, HINT_PADDING, iconTop, HINT_ICON_SIZE, HINT_ICON_SIZE);

		if (m_wHintBadgeText)
		{
			float badgeTextWidth, badgeTextHeight;
			MeasureText(m_wHintBadgeText, HINT_BADGE_FONT_SIZE, badgeTextWidth, badgeTextHeight);
			FrameSlot.SetPos(m_wHintBadgeText, HINT_PADDING + (HINT_ICON_SIZE - badgeTextWidth) * 0.5, iconTop + (HINT_ICON_SIZE - badgeTextHeight) * 0.5);
		}

		FrameSlot.SetPos(m_wHintText, HINT_PADDING + iconSpace, (HINT_HEIGHT - textHeight) * 0.5);
	}

	//------------------------------------------------------------------------------------------------
	//! Anchors for the three groups; sizes follow the content and are set while shown
	protected void ApplyLayout()
	{
		FrameSlot.SetAnchorMin(m_wRoot, 0, 0);
		FrameSlot.SetAnchorMax(m_wRoot, 1, 1);
		FrameSlot.SetOffsets(m_wRoot, 0, 0, 0, 0);
		m_wRoot.SetZOrder(ROOT_Z_ORDER);

		FrameSlot.SetAnchor(m_wHeadline, 0.5, HEADLINE_SCREEN_Y);
		FrameSlot.SetAlignment(m_wHeadline, 0.5, 0.5);
		FrameSlot.SetPos(m_wHeadline, 0, 0);

		FrameSlot.SetAnchor(m_wCard, 1, 0);
		FrameSlot.SetAlignment(m_wCard, 1, 0);
		FrameSlot.SetPos(m_wCard, -CARD_MARGIN, CARD_TOP);

		FrameSlot.SetAnchor(m_wHint, 0.5, 1);
		FrameSlot.SetAlignment(m_wHint, 0.5, 1);
		FrameSlot.SetPos(m_wHint, 0, -HINT_BOTTOM);

		array<Widget> texts = {m_wHeadlineText, m_wCardKicker, m_wCardText, m_wHintText, m_wHintBadgeText};
		foreach (Widget text : texts)
		{
			if (text)
				FrameSlot.SetSizeToContent(text, true);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Colours, fonts and shadows that do not change per message
	protected void ApplyStyle()
	{
		SetColor(m_wHeadlineBand, Color.FromRGBA(10, 12, 8, 200));
		SetColor(m_wCardBackground, Color.FromRGBA(16, 18, 14, 230));
		SetColor(m_wCardIcon, Color.FromRGBA(255, 255, 255, 255));
		StyleText(m_wHeadlineText, HEADLINE_FONT_SIZE, Color.FromRGBA(233, 234, 228, 255), true);
		StyleText(m_wCardKicker, CARD_KICKER_FONT_SIZE, Color.FromRGBA(178, 182, 170, 255), false);
		StyleText(m_wCardText, CARD_TEXT_FONT_SIZE, Color.FromRGBA(233, 234, 228, 255), true);
		StyleText(m_wHintText, HINT_FONT_SIZE, Color.FromRGBA(245, 245, 240, 255), true);

		// Dark "i" on the coloured badge
		if (m_wHintBadgeText)
		{
			m_wHintBadgeText.SetText("i");
			m_wHintBadgeText.SetExactFontSize(HINT_BADGE_FONT_SIZE);
			m_wHintBadgeText.SetColor(Color.FromRGBA(20, 18, 10, 255));
			m_wHintBadgeText.SetBold(true);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Single-line text size in reference pixels. GetTextSize returns 0 in game, so this reads the laid-out
	//! (size-to-content) widget instead, and estimates from the font size before the first layout pass.
	protected static void MeasureText(notnull TextWidget text, int fontSize, out float width, out float height)
	{
		float screenWidth, screenHeight;
		text.GetScreenSize(screenWidth, screenHeight);
		if (screenWidth > 0 && screenHeight > 0)
		{
			WorkspaceWidget workspace = GetGame().GetWorkspace();
			width = workspace.DPIUnscale(screenWidth);
			height = workspace.DPIUnscale(screenHeight);
			return;
		}

		string content = text.GetText();
		width = content.Length() * fontSize * 0.55;
		height = fontSize * 1.3;
	}

	//------------------------------------------------------------------------------------------------
	protected static void SetVisible(Widget widget, bool visible)
	{
		if (widget)
			widget.SetVisible(visible);
	}

	//------------------------------------------------------------------------------------------------
	//! Loads the icon, or hides the widget when there is none. Returns true if an icon is shown.
	protected static bool SetIcon(ImageWidget image, ResourceName icon)
	{
		if (!image)
			return false;

		bool hasIcon = !icon.IsEmpty();
		if (hasIcon)
			image.LoadImageTexture(0, icon);

		image.SetVisible(hasIcon);
		return hasIcon;
	}

	//------------------------------------------------------------------------------------------------
	protected static void PlaySound(string sound)
	{
		// Forced: unforced UI sounds are skipped while another UI sound plays (e.g. the click that closed a menu)
		if (!sound.IsEmpty())
			SCR_UISoundEntity.SoundEvent(sound, true);
	}

	//------------------------------------------------------------------------------------------------
	protected static void StyleText(TextWidget text, int fontSize, Color color, bool bold)
	{
		if (!text)
			return;

		text.SetExactFontSize(fontSize);
		text.SetColor(color);
		text.SetBold(bold);
		text.SetShadow(1, 0xFF000000, 0.5, 1, 1);
	}

	//------------------------------------------------------------------------------------------------
	protected static void SetColor(Widget widget, Color color)
	{
		if (widget && color)
			widget.SetColor(color);
	}

	//------------------------------------------------------------------------------------------------
	protected static void Place(Widget widget, float x, float y, float width, float height)
	{
		if (!widget)
			return;

		FrameSlot.SetPos(widget, x, y);
		FrameSlot.SetSize(widget, width, height);
	}
}
