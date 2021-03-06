#ifndef SimulateStepJob_H
#define SimulateStepJob_H
#include <qtphysicsunofficial_global.h>


namespace Physics {

class PhysicsManager;

class QTPHYSICSUNOFFICIAL_EXPORT SimulateStepJob:public Qt3DCore::QAspectJob
{
public:
    explicit SimulateStepJob(PhysicsManager* manager);
protected:
     void run() Q_DECL_OVERRIDE;
private:
    PhysicsManager* m_manager;

};
}
#endif // SimulateStepJob_H
