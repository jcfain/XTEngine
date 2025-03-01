#include "tcodefactory.h"

TCodeFactory::TCodeFactory(double inputStart, double inputEnd, QObject* parent) : QObject(parent)
{
    _input_start = inputStart;
    _input_end = inputEnd;
}
void TCodeFactory::init()
{
    _addedAxis->clear();
}

void TCodeFactory::calculate(QString axisName, double value, QVector<ChannelValueModel> &axisValues)
{
    if (axisName != TCodeChannelLookup::None()) {
        ChannelModel33* tcodeAxis = TCodeChannelLookup::getChannel(axisName);
        bool isNegative = tcodeAxis->AxisName.contains(TCodeChannelLookup::NegativeModifier);
        //auto isPositive = tcodeAxis.AxisName.contains(TCodeChannelLookup::PositiveModifier);
        if (_addedAxis->contains(tcodeAxis->Channel) && _addedAxis->value(tcodeAxis->Channel) == 0 && value != 0)
        {
            _addedAxis->remove(tcodeAxis->Channel);
            const ChannelValueModel cvm = boolinq::from(axisValues).firstOrDefault([tcodeAxis](const ChannelValueModel &x) { return x.Channel == tcodeAxis->Channel; });
            axisValues.removeOne(cvm);
        }
        if (!boolinq::from(axisValues).any([tcodeAxis](const ChannelValueModel &x) { return x.Channel == tcodeAxis->Channel; }))
        {
            double calculatedValue = value;
            if (isNegative && value > 0)
            {
                calculatedValue = -(value);
            }
            if (value != 0 && SettingsHandler::getChannelGamepadInverse(axisName))
            {
                calculatedValue = -(value);
            }
            axisValues.append({
                calculateTcodeRange(calculatedValue, tcodeAxis),
                tcodeAxis->Channel
            });
            _addedAxis->insert(tcodeAxis->Channel, value);
        }
    }
}

QString TCodeFactory::formatTCode(QVector<ChannelValueModel>* values)
{
    QString tCode = "";
    foreach (auto value, *values)
    {
        if(!value.Channel.isEmpty())
        {
            auto minValue = TCodeChannelLookup::getChannel(value.Channel)->Min;
            auto maxValue = TCodeChannelLookup::getChannel(value.Channel)->Max;
            auto clampedValue = maxValue == 0 ? value.Value : XMath::constrain(value.Value, minValue, maxValue);
            tCode += value.Channel + QString::number(clampedValue).rightJustified(SettingsHandler::getTCodePadding(), '0') + "S" + QString::number(SettingsHandler::getLiveGamepadSpeed()) + " ";
        }
    }
    return tCode.trimmed();
}

int TCodeFactory::calculateTcodeRange(double value, ChannelModel33* channel)
{
    auto parentChannel = TCodeChannelLookup::getChannel(channel->Channel);//Get parent as could be - or +
    int output_end = parentChannel->UserMax;
    int min = parentChannel->UserMin;
    // Update for live x range switch
    if(channel->Channel == TCodeChannelLookup::Stroke())
    {
        output_end = TCodeChannelLookup::getLiveXRangeMax();
        min = TCodeChannelLookup::getLiveXRangeMin();
    }
    int output_start = channel->Type != ChannelType::Ramp ? qRound((output_end + min) / 2.0) : min;
    double slope = (output_end - output_start) / (_input_end - _input_start);
    return qRound(output_start + slope * (value - _input_start));
}

int TCodeFactory::calculateGamepadSpeed(double gpIn)
{
    //return (int)(gpIn < 0 ? -gpIn * SettingsHandler::getSpeed : gpIn * SettingsHandler::getSpeed);
}
