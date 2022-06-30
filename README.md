## AwesomeHvH server plugin - legacy desync &amp; more

### Features:
- sv_legacy_desync - Disables recently added valve shitcode that breaks lag compensation ( [Changes to bonesetup in the recent update](https://www.unknowncheats.me/forum/counterstrike-global-offensive/471626-changes-bonesetup-recent-update.html#post3256352) ).
- sv_disable_lagpeek - Disables lagpeek aka "defensive doubletap".
- sv_disable_roll_aa - Disables extended roll desync that was added by some uneducated valve employee ( ["Lean antiaim/Extended desync" exploit](https://www.unknowncheats.me/forum/counterstrike-global-offensive/483295-lean-antiaim-extended-desync-exploit.html) ).
- sv_force_lag_compensation - Forces lagcompensation to avoid anti-exploiters with cl_lagcompensation set to 0.

### Dependencies:
sv_legacy_desync requires replicated convar on client ( [easy convar creation](https://www.unknowncheats.me/forum/counterstrike-global-offensive/317654-easy-convar-creation.html) ) with this hook:
```cpp
// 55 8B EC 83 E4 F8 83 EC 70 56 57 8B F9 89 7C 24 38 83 BF ? ? ? ? ? 75
void __fastcall hkClampBonesInBBox(CCSPlayer* player, void*, matrix3x4_t* matrix, int mask)
{
    if (!sv_legacy_desync->GetBool())
        oClampBonesInBBox(player, matrix, mask);
}
```

### Installation:
  ![alt text](https://i.imgur.com/PU7IeIe.png)
  ![alt text](https://i.imgur.com/SA1U2xp.png)

### Topics:
[yougame.biz](https://yougame.biz/threads/241340/)

[unknowncheats.me](https://www.unknowncheats.me/forum/counterstrike-global-offensive/487084-awesomehvh-server-plugin-legacy-desync.html)
