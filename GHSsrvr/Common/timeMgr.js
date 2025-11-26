
// Requies no params. Returns object with current Date and time down to the
// millisecond value. Inner methods will return strings of datestamps, 
// timestamps, as well as both date and time stamps.
const getTime = function() {

    const _T = new Date();
    this.rawTime = _T.getTime();
    this.year = _T.getFullYear();
    this.month = _T.getMonth() + 1; // Zero indexed.
    this.day = _T.getDate();
    this.hour = _T.getHours();
    this.min = _T.getMinutes();
    this.sec = _T.getSeconds();
    this.ms = _T.getMilliseconds();

    // Pads values to enforce yyyymmdd hh:mm:ss format if below 10.
    const _pad = function(n) {
        let padVal = n < 10 ? "0" + n : "" + n;
        return padVal;
    }

    // Gets datestamp string only.
    this.getDatestamp = function() {
        return `${this.year}-${_pad(this.month)}-${_pad(this.day)}`;
    }

    // Gets timestamp string only, does not include ms.
    this.getTimestamp = function() {
        return `${_pad(this.hour)}:${_pad(this.min)}:${_pad(this.sec)}`;
    }

    // Returns timestamp used for log entries.
    this.getDateTimestamp = function() {
        return `${this.getDatestamp()} ${this.getTimestamp()}`;
    }

    // Returns object including the current day's hours, minutes, seconds,
    // and ms.
    this.getDayBreakdown = function() {

        const min = (this.hour * 60) + this.min;
        const sec = (min * 60) + this.sec;
        const ms = (sec * 1000) + this.ms;

        const data = {"hour": this.hour, "min": min, "sec": sec, "ms": ms};
        return data;
    }

    // Returns object including the current hour's minutes, seconds, and ms.
    this.getHourBreakdown = function() {

        const sec = (this.min * 60) + this.sec;
        const ms = (sec * 1000) + this.ms;

        const data = {"min": this.min, "sec": sec, "ms": ms};
        return data;
    }

    return this;
}

module.exports = {getTime};