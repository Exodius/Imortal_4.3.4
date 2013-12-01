#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "hour_of_twilight.h"

#define SAY_AGGRO "Give up the Dragon Soul, and I may yet allow you to live."
#define SAY_DEATH "Nothing! Nothing..."
#define SAY_PLAYER_KILL "Your shaman can't protect you."
#define SAY_ICE_TOMB "Enough, shaman!"
#define SAY_PHASE2 "The Hour of Twilight falls - the end of all things - you cannot stop it. You are nothing. NOTHING!"

enum Spells
{
    SPELL_HAND_OF_FROST           = 102593,
    SPELL_TORRENT_OF_FROST        = 103962,
    SPELL_CHAINS_OF_FROST         = 102582,
    SPELL_ICY_TOMB                = 103252,

    SPELL_ICY_BOULDER             = 107848,

    SPELL_LAVA_BURST              = 107980,
};

enum Events
{
    EVENT_HAND_OF_FROST           = 1,
    EVENT_CHAINS_OF_FROST         = 2,
    EVENT_ICY_TOMB                = 3,
    EVENT_SUMMON                  = 4,

    EVENT_ICY_BOULDER             = 5,

    EVENT_LAVA_BURST              = 6,
};

enum EncounterActions
{
    ACTION_START_ATACK            = 1,
}

Position const SpawnPositions[5] =
{
    {0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f},
};

class boss_arcurion : public CreatureScript
{
    public:

        boss_arcurion() : CreatureScript("boss_arcurion") { }

        struct boss_arcurionAI : public BossAI
        {
            boss_arcurionAI(Creature* creature) : BossAI(creature, DATA_ARCURION) { }

            void Reset() OVERRIDE
            {
                events.Reset();

                _Reset();
            }

            void EnterCombat(Unit* /*who*/) OVERRIDE
            {
                events.ScheduleEvent(EVENT_HAND_OF_FROST, 2000);
                events.ScheduleEvent(EVENT_CHAINS_OF_FROST, 13000);
                events.ScheduleEvent(EVENT_ICY_TOMB, 30000);
                events.ScheduleEvent(EVENT_SUMMON, 5000);

                Talk(SAY_AGGRO);
                _EnterCombat();
            }

            void JustDied(Unit* /*killer*/) OVERRIDE
            {
                Talk(SAY_DEATH);
                _JustDied();
            }

            void KilledUnit(Unit* victim) OVERRIDE
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_PLAYER_KILL);
            }

            void DamageTaken(Unit* /*attacker*/, uint32& damage) OVERRIDE
            {
                if (me->HealthBelowPctDamaged(30, damage))
                {
                    DoCastAOE(SPELL_TORRENT_OF_FROST);
                    Talk(SAY_PHASE2);
                }
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_HAND_OF_FROST:
                               DoCastVictim(SPELL_HAND_OF_FROST);
                               events.ScheduleEvent(EVENT_HAND_OF_FROST, 2000);
                            break;

                        case EVENT_CHAINS_OF_FROST:
                               DoCastAOE(SPELL_CHAINS_OF_FROST);
                               events.ScheduleEvent(EVENT_CHAINS_OF_FROST, 13000);
                            break;

                        case EVENT_ICY_TOMB:
                               if (Creature* thrall = me->FindNearestCreature(NPC_THRALL, 50.0f))
                                   DoCast(thrall, SPELL_ICY_TOMB);
                                   Talk(SAY_ICE_TOMB);
                               events.ScheduleEvent(EVENT_ICY_TOMB, 30000);
                            break;

                        case EVENT_SUMMON:
                                me->SummonCreature(NPC_FROZEN_SERVITOR, SpawnPositions[urand(1,5)]);
                                if (Creature* thrall = me->FindNearestCreature(NPC_THRALL, 50.0f))
                                    Thrall->AI()->DoAction(ACTION_START_ATACK);
                                events.ScheduleEvent(EVENT_SUMMON, 10000);
                            break;

                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return GetHourOfTwilightAI<boss_arcurionAI>(creature);
        }
};

class npc_frozen_servitor : public CreatureScript
{
    public:
        npc_frozen_servitor() : CreatureScript("npc_frozen_servitor") { }

        struct npc_frozen_servitorAI : public ScriptedAI
        {
            npc_frozen_servitorAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            void IsSummonedBy(Unit* /*summoner*/) OVERRIDE
            {

                events.ScheduleEvent(EVENT_ICY_BOULDER, 4000);

            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_ICY_BOULDER:
                               if(Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                                   DoCast(target, SPELL_ICY_BOULDER);
                            events.ScheduleEvent(EVENT_ICY_BOULDER, 4000);
                            break;

                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();


        private:
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new frozen_servitorAI(creature);
        }
};

class npc_thrall : public CreatureScript
{
    public:
        npc_thrall() : CreatureScript("npc_thrall") { }

        struct npc_thrallAI : public ScriptedAI
        {
            npc_thrallAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            void DoAction(int32 action) OVERRIDE
            {
                switch (action)
                {
                    case ACTION_START_ATACK:
                        events.ScheduleEvent(EVENT_LAVA_BURST, 2000);
                        break;

                    default:
                        break;
                }
            }

            void UpdateAI(uint32 diff) OVERRIDE
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_LAVA_BURST:
                               if (Creature* frozen = me->FindNearestCreature(NPC_FROZEN_SERVITOR, 100.0f))
                                   DoCast(frozen, SPELL_LAVA_BURST);
                               events.ScheduleEvent(EVENT_LAVA_BURST, 2000);
                            break;

                        default:
                            break;
                    }
                }
            }

        private:
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const OVERRIDE
        {
            return new npc_thrallAI(creature);
        }
};

void AddSC_boss_arcurion()
{
    new boss_arcurion();
    new npc_thrall();
    new npc_frozen_servitor();
}