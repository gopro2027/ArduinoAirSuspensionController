#include "sampleLoggingUtil.h"

uint8_t AIPercentage = 0;

// Re-fit one model's weights from its stored samples (closed-form ridge least squares, ~1 ms). This is
// the ONLY training path: no online learner, nothing persisted — more samples just means another refit.
void refitModel(SOLENOID_AI_INDEX index)
{
    OffsetModel *m = getOffsetModel(index);
    int len = getLearnDataLength(index);
    int used = m->refit(getLearnData(index), len);
    Serial.printf("Refit model %i: %i samples, used %i  w=[%.4f %.4f %.4f]\n",
                  (int)index, len, used, m->w[0], m->w[1], m->w[2]);
}

// Refit any model whose stored sample count changed since the last call. Called once at boot (refits all,
// since lastCount seeds to -1) and then every 100 ms from task_trainAI.
void trainOffsetModels()
{
    static int lastCount[4] = {-1, -1, -1, -1};
    for (int i = 0; i < 4; i++)
    {
        int c = getLearnDataLength((SOLENOID_AI_INDEX)i);
        if (c != lastCount[i])
        {
            refitModel((SOLENOID_AI_INDEX)i);
            lastCount[i] = c;
        }
    }
    updateAIPercentage();
}

void updateAIPercentage()
{
    // Progress toward full AI weighting (100% == every model at AI_LEARN_RATIO_NUM samples, i.e.
    // AI fully driving), not toward the LEARN_SAVE_COUNT collection ceiling. See AI_TRAINING.md.
    int totalLen = 0;
    for (int i = 0; i < 4; i++)
    {
        int len = getLearnDataLength((SOLENOID_AI_INDEX)i);
        totalLen += len < AI_LEARN_RATIO_NUM ? len : AI_LEARN_RATIO_NUM;
    }
    AIPercentage = ((float)totalLen / ((float)AI_LEARN_RATIO_NUM * 4)) * 100;
}

// Fraction of the trained model to trust, ramping in with sample count: 0 below OFFSET_FADE_MIN, linearly
// up to 1 at AI_LEARN_RATIO_NUM. Below the min, the flat constant default is used (-/+ OFFSET_DEFAULT_PSI
// for up/down). Raw flowing readings alone are wrong by ~10 psi, so an offset is always applied.
static double getPredictionOffset(SOLENOID_AI_INDEX aiIndex, double raw_bag, double raw_tank)
{
    OffsetModel *m = getOffsetModel(aiIndex);
    double def = m->up ? -(double)OFFSET_DEFAULT_PSI : (double)OFFSET_DEFAULT_PSI;
    int count = getLearnDataLength(aiIndex);
    if (count < OFFSET_FADE_MIN)
    {
        return def;
    }
    double trained = m->predict(raw_bag, raw_tank);
    double w = (double)(count - OFFSET_FADE_MIN) / (double)(AI_LEARN_RATIO_NUM - OFFSET_FADE_MIN);
    if (w > 1.0)
    {
        w = 1.0;
    }
    return def * (1.0 - w) + trained * w;
}

// Predicted true bag pressure recovered from the live flowing readings while a valve is open. The
// closed-loop controller stops when this reaches goal. See AI_TRAINING.md.
double getPredictedBagPressure(SOLENOID_AI_INDEX aiIndex, double raw_bag, double raw_tank)
{
    return raw_bag + getPredictionOffset(aiIndex, raw_bag, raw_tank);
}

// Height-mode counterpart to getPredictedBagPressure. The level sensor reads the true ride height even
// while a valve is open, so this is an identity stub — it exists so the closed-loop controller can treat
// both modes through one predictor seam. (A future valve-close-lag model could live here; see the
// "Future improvements" note in AI_TRAINING.md.)
double getPredictedBagHeight(double raw_level)
{
    return raw_level;
}
