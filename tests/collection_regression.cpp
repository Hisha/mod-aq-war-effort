
int main(){
 using namespace AQWarEffort;
 objectMgr.quests[8517]=Quest{8517, {1251}, {20}};
objectMgr.quests[8518]=Quest{8518, {1251}, {20}};
objectMgr.quests[8520]=Quest{8520, {6450}, {20}};
objectMgr.quests[8521]=Quest{8521, {6450}, {20}};
objectMgr.quests[8522]=Quest{8522, {14529}, {20}};
objectMgr.quests[8523]=Quest{8523, {14529}, {20}};
objectMgr.quests[8524]=Quest{8524, {5095}, {20}};
objectMgr.quests[8525]=Quest{8525, {5095}, {20}};
objectMgr.quests[8526]=Quest{8526, {12210}, {20}};
objectMgr.quests[8527]=Quest{8527, {12210}, {20}};
objectMgr.quests[8528]=Quest{8528, {6887}, {20}};
objectMgr.quests[8529]=Quest{8529, {6887}, {20}};
objectMgr.quests[8503]=Quest{8503, {3820}, {20}};
objectMgr.quests[8504]=Quest{8504, {3820}, {20}};
objectMgr.quests[8509]=Quest{8509, {8836}, {20}};
objectMgr.quests[8510]=Quest{8510, {8836}, {20}};
objectMgr.quests[8505]=Quest{8505, {8831}, {20}};
objectMgr.quests[8506]=Quest{8506, {8831}, {20}};
objectMgr.quests[8494]=Quest{8494, {3575}, {20}};
objectMgr.quests[8495]=Quest{8495, {3575}, {20}};
objectMgr.quests[8499]=Quest{8499, {12359}, {20}};
objectMgr.quests[8500]=Quest{8500, {12359}, {20}};
objectMgr.quests[8492]=Quest{8492, {2840}, {20}};
objectMgr.quests[8493]=Quest{8493, {2840}, {20}};
objectMgr.quests[8511]=Quest{8511, {2318}, {10}};
objectMgr.quests[8512]=Quest{8512, {2318}, {10}};
objectMgr.quests[8513]=Quest{8513, {2319}, {10}};
objectMgr.quests[8514]=Quest{8514, {2319}, {10}};
objectMgr.quests[8515]=Quest{8515, {4304}, {10}};
objectMgr.quests[8516]=Quest{8516, {4304}, {10}};
objectMgr.quests[8604]=Quest{8604, {3530}, {20}};
objectMgr.quests[8605]=Quest{8605, {3530}, {20}};
objectMgr.quests[8607]=Quest{8607, {8544}, {20}};
objectMgr.quests[8608]=Quest{8608, {8544}, {20}};
objectMgr.quests[8609]=Quest{8609, {14529}, {20}};
objectMgr.quests[8610]=Quest{8610, {14529}, {20}};
objectMgr.quests[8611]=Quest{8611, {12209}, {20}};
objectMgr.quests[8612]=Quest{8612, {12209}, {20}};
objectMgr.quests[8615]=Quest{8615, {13935}, {20}};
objectMgr.quests[8616]=Quest{8616, {13935}, {20}};
objectMgr.quests[8613]=Quest{8613, {6887}, {20}};
objectMgr.quests[8614]=Quest{8614, {6887}, {20}};
objectMgr.quests[8549]=Quest{8549, {2447}, {20}};
objectMgr.quests[8550]=Quest{8550, {2447}, {20}};
objectMgr.quests[8580]=Quest{8580, {4625}, {20}};
objectMgr.quests[8581]=Quest{8581, {4625}, {20}};
objectMgr.quests[8582]=Quest{8582, {8831}, {20}};
objectMgr.quests[8583]=Quest{8583, {8831}, {20}};
objectMgr.quests[8542]=Quest{8542, {3576}, {20}};
objectMgr.quests[8543]=Quest{8543, {3576}, {20}};
objectMgr.quests[8545]=Quest{8545, {3860}, {20}};
objectMgr.quests[8546]=Quest{8546, {3860}, {20}};
objectMgr.quests[8532]=Quest{8532, {2840}, {20}};
objectMgr.quests[8533]=Quest{8533, {2840}, {20}};
objectMgr.quests[8588]=Quest{8588, {4234}, {10}};
objectMgr.quests[8589]=Quest{8589, {4234}, {10}};
objectMgr.quests[8600]=Quest{8600, {8170}, {10}};
objectMgr.quests[8601]=Quest{8601, {8170}, {10}};
objectMgr.quests[8590]=Quest{8590, {4304}, {10}};
objectMgr.quests[8591]=Quest{8591, {4304}, {10}};

 auto& m=Manager::Instance(); Player a,h;h.faction=1;
 events.changed=[&](){m.SyncCollectionEvent();};
 auto reward=[&](uint32 id){m.OnQuestReward(&a,objectMgr.GetQuestTemplate(id));};
 m.Initialize();assert(m.IsAvailable()&&events.active&&events.starts==1);
 m.SyncCollectionEvent();m.Initialize();assert(events.starts==1&&events.stops==0);
 reward(8511);assert(CharacterDatabase.rows[1].Contributions[0][12]==10);assert(m.Scores(0).find("Light Leather: 10 / 40")!=std::string::npos);
 reward(8512);assert(CharacterDatabase.rows[1].Contributions[0][12]==20);
 for(uint8 f=0;f<2;++f)for(uint8 i=0;i<15;++i){auto const& mat=m.GetMaterial(f,i);reward(mat.Quest);reward(mat.RepeatQuest);}
 assert(CharacterDatabase.rows[1].Contributions[1][0]==40); // classified by quest, even with Alliance player
 assert(m.GetPhase()==AQ_PHASE_WAR_EFFORT);
 for(uint8 f=0;f<2;++f)for(uint8 i=0;i<15;++i){auto const& mat=m.GetMaterial(f,i);while(CharacterDatabase.rows[1].Contributions[f][i]<40)reward(mat.RepeatQuest);}
 assert(m.IsComplete()&&m.GetPhase()==AQ_PHASE_READY&&!events.active&&events.stops==1);
 m.SyncCollectionEvent();assert(events.stops==1);
 auto full=CharacterDatabase.rows[1];reward(8511);assert(CharacterDatabase.rows[1]==full);
 assert(m.SetPhase(AQ_PHASE_TEN_HOUR_WAR)&&!events.active);assert(CharacterDatabase.rows[1].GongRungAt==0);
 assert(m.SetPhase(AQ_PHASE_OPEN)&&!events.active);auto opened=CharacterDatabase.rows[1];m.Initialize();assert(CharacterDatabase.rows[1]==opened&&!events.active);
 assert(m.SetPhase(AQ_PHASE_DISABLED)&&!events.active);assert(m.SetPhase(AQ_PHASE_WAR_EFFORT)&&events.active);
 events.StopEvent(22,false);assert(events.active); // correct external stop without recursion
 assert(m.SetPhase(AQ_PHASE_READY));events.StartEvent(22,false);assert(!events.active); // correct external start
 config.goal=5;auto before=CharacterDatabase.rows[1];m.Initialize();assert(!m.IsAvailable()&&!events.active&&CharacterDatabase.rows[1]==before);
 for(uint32 bad: {0,5,25}){config.goal=bad;m.Initialize();assert(!m.IsAvailable());}
 config.goal=40;for(auto bad: {"no", "-1", "18446744073709551616"}){config.rawGoal=bad;m.Initialize();assert(!m.IsAvailable());}config.rawGoal.clear();
 config.keys={"AQWarEffort.Goal.Alliance.LightLeather"};m.Initialize();assert(!m.IsAvailable());config.keys.clear();
 objectMgr.quests[8511].RequiredItemCount[0]=11;m.Initialize();assert(!m.IsAvailable());objectMgr.quests[8511].RequiredItemCount[0]=10;
 CharacterDatabase.version=1;m.Initialize();assert(!m.IsAvailable()&&CharacterDatabase.rows[1]==before);CharacterDatabase.version=2;
 config.id=2;m.Initialize();assert(events.active);CharacterDatabase.failWrite=true;reward(8511);assert(!m.IsAvailable()&&!events.active&&CharacterDatabase.rows[2].Contributions[0][12]==0);CharacterDatabase.failWrite=false;
 m.Initialize();reward(8511);assert(CharacterDatabase.rows[2].Contributions[0][12]==10);
 config.enabled=false;m.Initialize();auto saved=CharacterDatabase.rows[2];reward(8511);assert(CharacterDatabase.rows[2]==saved);config.enabled=true;
 config.id=3;m.Initialize();for(auto& faction:CharacterDatabase.rows[3].Contributions)for(auto& count:faction)count=40;CharacterDatabase.hasCampaign[3]=false;m.Initialize();assert(m.GetPhase()==AQ_PHASE_READY);
 assert(m.SetPhase(AQ_PHASE_WAR_EFFORT));m.Initialize();assert(m.GetPhase()==AQ_PHASE_WAR_EFFORT);reward(8511);assert(m.GetPhase()==AQ_PHASE_READY);
 config.id=4;m.Initialize();CharacterDatabase.rows[4].Contributions[0][12]=std::numeric_limits<uint64>::max()-5;m.Initialize();reward(8511);assert(CharacterDatabase.rows[4].Contributions[0][12]==std::numeric_limits<uint64>::max()-5);
}
