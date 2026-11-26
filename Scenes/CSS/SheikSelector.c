#include "SheikSelector.h"

static u8 *GetCSSDTBytes() {
  return (u8 *)GetSlpCSSDT();
}

bool IsNativeLocalTeams() {
  u8 *dt = GetCSSDTBytes();
  return dt != NULL && dt[CSSDT_LOCAL_TEAMS_STATUS + 9] != 0 && dt[CSSDT_NATIVE_INITIALIZED] != 0;
}

u8 GetNativeVisibleCount() {
  u8 *dt = GetCSSDTBytes();
  if (dt == NULL) return 0;
  u8 count = dt[CSSDT_NATIVE_VISIBLE_COUNT];
  if (count < 1) return 1;
  if (count > 4) return 4;
  return count;
}

u8 GetPlayerIndex() {
  return R13_U8(PLAYER_IDX_R13_OFFSET);
}

CSSData *GetCSSData() {
  return R13_PTR(CSS_DATA_R13_OFFSET);
}

void SetSelectedChar(u8 ckind) {
  u8 playerIndex = GetPlayerIndex();
  CSSData *CssData = GetCSSData();
  CssData->data.data.players[playerIndex].c_kind = ckind;

  // Change nametag to say Sheik or Zelda
  CSSIcon *iconData = 0x803f0cc8;
  iconData->char_kind = ckind;

  SFX_getCharacterNameAnnouncer(ckind);
  CSS_CursorHighlightUpdateCSPInfo(0);
}

void SetSelectedCharForSlot(u8 slot, u8 ckind) {
  CSSData *CssData = GetCSSData();
  CssData->data.data.players[slot].c_kind = ckind;

  // Change nametag/CSP to say Sheik or Zelda for this slot, then restore Sheik
  // as Slippi's icon default. Mixed Zelda/Sheik slots are preserved across CSS reloads
  // by NativeLocalTeamsCount + InitNativeLocalTeams.
  CSSIcon *iconData = 0x803f0cc8;
  iconData->char_kind = ckind;
  CSS_CursorHighlightUpdateCSPInfo(slot);
  iconData->char_kind = CKIND_SHEIK;

  SFX_getCharacterNameAnnouncer(ckind);
}

u8 GetSelectedChar() {
  u8 selectedChar = ACCESS_U8(0x8043208f);
  return selectedChar;
}

Vec2 GetCursorPos() {
  CSSPlayerData *cssPlayerData = ACCESS_PTR(0x804A0BC0);
  return cssPlayerData->position;
}

void InitSheikSelector() {
  SlippiCSSDataTable *dt = GetSlpCSSDT();
  if (dt == NULL || dt->SlpCSSDatAddress == NULL) {
    selectorCount = 0;
    return;
  }

  SlpCSSDesc *slpCss = dt->SlpCSSDatAddress;
  for (u8 i = 0; i < 4; i++) {
    selectorGobjs[i] = NULL;
    selectorJobjs[i] = NULL;
    selectorScales[i] = 1.f;
  }

  if (!IsNativeLocalTeams()) {
    selectorCount = 1;
    selectorGobjs[0] = GObj_Create(0x4, 0x5, 0x80);
    selectorJobjs[0] = JOBJ_LoadJoint(slpCss->sheikSelector);

    selectorJobjs[0]->trans.X = -8.f;
    selectorJobjs[0]->trans.Y = -22.5f;

    JOBJ_SetMtxDirtySub(selectorJobjs[0]);

    GObj_AddObject(selectorGobjs[0], 0x4, selectorJobjs[0]);
    GObj_AddGXLink(selectorGobjs[0], GXLink_Common, 1, 129);

    isInNameEntry = IsOnCSSNameEntryScreen();
    return;
  }

  static const float cardScales[4] = {1.0f, 1.0f, 0.70f, 0.53f};
  static const float origCx[4] = {-23.1f, -7.7f, 7.7f, 23.1f};
  static const float shifts[4][4] = {
      {0.0f, 0.0f, 0.0f, 0.0f},
      {0.0f, 2.6f, 5.2f, 7.8f},
      {-2.175f, -6.4f, -10.625f, 0.0f},
      {-3.4075f, -10.5358f, -17.6642f, -24.7925f},
  };

  u8 count = GetNativeVisibleCount();
  selectorCount = count;
  float s = cardScales[count - 1];

  u8 *dtBytes = (u8 *)dt;
  Text **nameTexts = (Text **)(dtBytes + CSSDT_CARD_NAME_TEXTS);
  float *origZ = (float *)(dtBytes + CSSDT_NATIVE_ORIGINAL_Z);

  for (u8 slot = 0; slot < count; slot++) {
    selectorGobjs[slot] = GObj_Create(0x4, 0x5, 0x80);
    selectorJobjs[slot] = JOBJ_LoadJoint(slpCss->sheikSelector);

    float cx;
    float cy;
    if (nameTexts[slot] != NULL) {
      cx = nameTexts[slot]->trans.X + 5.52f * s;
      cy = -nameTexts[slot]->trans.Y - 3.10f * s;
    } else {
      cx = origCx[slot] + shifts[count - 1][slot];
      cy = -3.4f - 17.25f * s;
    }

    float cardZ = origZ[slot] + (count > 2 ? -0.03f * (float)slot : 0.0f);

    selectorJobjs[slot]->trans.X = cx;
    selectorJobjs[slot]->trans.Y = cy;
    selectorJobjs[slot]->trans.Z = cardZ + 0.05f;
    selectorJobjs[slot]->scale.X = 1.5f * s;
    selectorJobjs[slot]->scale.Y = 1.5f * s;
    selectorJobjs[slot]->scale.Z = 1.0f;
    selectorScales[slot] = s;

    JOBJ_SetMtxDirtySub(selectorJobjs[slot]);
    JOBJ_SetAllAlpha(selectorJobjs[slot], 0.f);

    GObj_AddObject(selectorGobjs[slot], 0x4, selectorJobjs[slot]);
    GObj_AddGXLink(selectorGobjs[slot], GXLink_Common, 1, 129);
  }

  isInNameEntry = IsOnCSSNameEntryScreen();
}

void UpdateSheikSelector() {
  bool wasInNameEntry = isInNameEntry;
  isInNameEntry = IsOnCSSNameEntryScreen();

  if (isInNameEntry) {
    return;
  }

  if (wasInNameEntry) {
    // When we come back from name entry, we need to re-init our gobj.
    // All Gobj's get destroyed by fn 8022f138 when entering name entry so we shouldn't be
    // leaking memory by re-initializing here
    InitSheikSelector();
  }

  if (!IsNativeLocalTeams()) {
    JOBJ *selectorJobj = selectorJobjs[0];
    if (selectorJobj == NULL) return;

    // If sheik / zelda not selected, hide selector UI
    u8 selectedChar = GetSelectedChar();
    if (selectedChar != CKIND_SHEIK && selectedChar != CKIND_ZELDA) {
      // Hide selector UI
      JOBJ_SetAllAlpha(selectorJobj, 0.f);
      return;
    }

    bool isLockedIn = SLIPPI_CSS_DATA_REF->dt->msrb->is_local_player_ready;
    if (isLockedIn) {
      // Show selector but dont let it be interactable while we are locked in.
      // We still need to update the alphas cause if we just came from name entry
      // both would show up as selected if we didn't run this logic. By passing false
      // twice though we avoid any potential hover states from showing.
      JOBJ_SetAllAlpha(selectorJobj, 1.f);
      UpdateSelectorAlphas(false, false);
      return;
    }

    // Show selector UI
    JOBJ_SetAllAlpha(selectorJobj, 1.f);

    // Get inputs and cursor pos
    u8 port = GetPlayerIndex();
    u64 downInputs = Pad_GetDown(port);
    Vec2 cursorPos = GetCursorPos();

    bool sheikHovered = false;
    bool zeldaHovered = false;

    for (int i = 0; i < 2; i++) {
      bool isSheik = i == 1;

      // Check if cursor is at the height of the selector
      const float BUTTON_TOP = SELECTOR_CENTER_Y + (TAP_TARGET_HEIGHT / 2.0f);
      const float BUTTON_BOTTOM = SELECTOR_CENTER_Y - (TAP_TARGET_HEIGHT / 2.0f);
      if (cursorPos.Y > BUTTON_TOP || cursorPos.Y < BUTTON_BOTTOM) {
        continue;
      }

      // Check if cursor is within the bounds of this button
      const float BUTTON_LEFT = SELECTOR_CENTER_X - (!isSheik ? TAP_TARGET_WIDTH : 0);
      const float BUTTON_RIGHT = SELECTOR_CENTER_X + (isSheik ? TAP_TARGET_WIDTH : 0);
      if (cursorPos.X < BUTTON_LEFT || cursorPos.X > BUTTON_RIGHT) {
        continue;
      }

      // Manage hovered flags
      sheikHovered = isSheik;
      zeldaHovered = !isSheik;

      // Now check for button press to see if we should change characters
      bool aButtonPressed = downInputs & HSD_BUTTON_A;
      if (!aButtonPressed) {
        continue;
      }

      // A was pressed, change character
      SetSelectedChar(isSheik ? CKIND_SHEIK : CKIND_ZELDA);
    }

    UpdateSelectorAlphas(zeldaHovered, sheikHovered);
    return;
  }

  SlippiCSSDataTable *dt = GetSlpCSSDT();
  u8 *dtBytes = (u8 *)dt;
  u8 *lts = dtBytes + CSSDT_LOCAL_TEAMS_STATUS;
  u8 confirmedMask = lts[4];
  u8 phase = lts[5];
  u8 joinedMask = lts[32];
  u8 suppressedMask = lts[33];

  CSSData *cssData = GetCSSData();
  CSSPlayerData **cursors = (CSSPlayerData **)0x804A0BC0;
  CSSPuck **pucks = (CSSPuck **)0x804A0BD0;

  for (u8 slot = 0; slot < selectorCount; slot++) {
    JOBJ *jobj = selectorJobjs[slot];
    if (jobj == NULL) continue;

    bool isClaimed = (joinedMask & (1 << slot)) != 0;
    bool isPlaced = pucks[slot] != NULL && pucks[slot]->state == 0;
    u8 selectedChar = (u8)cssData->data.data.players[slot].c_kind;

    if (!isClaimed || !isPlaced || (selectedChar != CKIND_SHEIK && selectedChar != CKIND_ZELDA)) {
      JOBJ_SetAllAlpha(jobj, 0.f);
      continue;
    }

    JOBJ_SetAllAlpha(jobj, 1.f);

    bool isLocked = (phase != 0) || ((confirmedMask & (1 << slot)) != 0) ||
                    ((suppressedMask & (1 << slot)) != 0) || dt->chatWindowOpened ||
                    (dt->msrb != NULL && dt->msrb->is_local_player_ready);
    if (isLocked || cursors[slot] == NULL) {
      UpdateSelectorAlphasForSlot(jobj, selectedChar, false, false);
      continue;
    }

    Vec2 cursorPos = cursors[slot]->position;
    u64 downInputs = Pad_GetDown(slot);
    float s = selectorScales[slot];

    float centerX = jobj->trans.X - 5.0f;
    float centerY = jobj->trans.Y + 1.25f;
    float btnW = TAP_TARGET_WIDTH * s;
    float btnH = TAP_TARGET_HEIGHT * s;

    bool sheikHovered = false;
    bool zeldaHovered = false;

    for (int i = 0; i < 2; i++) {
      bool isSheik = i == 1;

      const float BUTTON_TOP = centerY + (btnH / 2.0f);
      const float BUTTON_BOTTOM = centerY - (btnH / 2.0f);
      if (cursorPos.Y > BUTTON_TOP || cursorPos.Y < BUTTON_BOTTOM) {
        continue;
      }

      const float BUTTON_LEFT = centerX - (!isSheik ? btnW : 0.f);
      const float BUTTON_RIGHT = centerX + (isSheik ? btnW : 0.f);
      if (cursorPos.X < BUTTON_LEFT || cursorPos.X > BUTTON_RIGHT) {
        continue;
      }

      sheikHovered = isSheik;
      zeldaHovered = !isSheik;

      if (downInputs & HSD_BUTTON_A) {
        u8 nextChar = isSheik ? CKIND_SHEIK : CKIND_ZELDA;
        SetSelectedCharForSlot(slot, nextChar);
        selectedChar = nextChar;
      }
    }

    UpdateSelectorAlphasForSlot(jobj, selectedChar, zeldaHovered, sheikHovered);
  }
}

void UpdateSelectorAlphasForSlot(JOBJ *jobj, u8 selectedChar, bool zeldaHovered, bool sheikHovered) {
  JOBJ *zeldaIcon = jobj->child;
  JOBJ *sheikIcon = jobj->child->sibling;

  bool sheikSelected = selectedChar == CKIND_SHEIK;
  bool zeldaSelected = selectedChar == CKIND_ZELDA;

  float zeldaAlpha = INACTIVE_ALPHA;
  float sheikAlpha = INACTIVE_ALPHA;

  if (zeldaHovered) {
    zeldaAlpha = HOVER_ALPHA;
  }
  if (sheikHovered) {
    sheikAlpha = HOVER_ALPHA;
  }

  if (zeldaSelected) {
    zeldaAlpha = ACTIVE_ALPHA;
  }
  if (sheikSelected) {
    sheikAlpha = ACTIVE_ALPHA;
  }

  JOBJ_SetAllAlpha(zeldaIcon, zeldaAlpha);
  JOBJ_SetAllAlpha(sheikIcon, sheikAlpha);
}

void UpdateSelectorAlphas(bool zeldaHovered, bool sheikHovered) {
  UpdateSelectorAlphasForSlot(selectorJobjs[0], GetSelectedChar(), zeldaHovered, sheikHovered);
}
