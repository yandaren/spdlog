/**
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 * 
 * hourly_file_sink.hpp
 *
 * a extend of file sink, auto rotating file houly
 *
 * @author  :   yandaren1220@126.com
 * @date    :   2017-04-11
 */

#pragma once

namespace spdlog
{
namespace sinks
{
namespace extend
{

/*
 * Generator of daily log file names in format basename.YYYY-MM-DD.extension
 */
struct houronly_daily_file_name_calculator
{
    // Create filename for the form basename.YYYY-MM-DD
    static filename_t calc_filename(const filename_t& filename)
    {
        std::tm tm = spdlog::details::os::localtime();
        filename_t basename, ext;
        std::tie(basename, ext) = details::file_helper::split_by_extenstion(filename);
        std::conditional<std::is_same<filename_t::value_type, char>::value, fmt::MemoryWriter, fmt::WMemoryWriter>::type w;
        w.write(SPDLOG_FILENAME_T("{}_{:04d}-{:02d}-{:02d}-{:02d}{}"), basename, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, ext);
        return w.str();
    }
};

/*
 * Rotating file sink based on date time. rotates at every hour
 */
template<class Mutex, class FileNameCalc = houronly_daily_file_name_calculator>
class hourly_file_sink SPDLOG_FINAL :public base_sink < Mutex >
{
public:
    //create daily file sink which rotates on given time
    hourly_file_sink(
        const filename_t& base_filename) 
    	: _base_filename(base_filename)
		, _force_flush(false)
    {
        _rotation_tp = _next_rotation_tp();
        _file_helper.open(FileNameCalc::calc_filename(_base_filename));
    }

	void set_force_flush(bool force_flush)
	{
		_force_flush = force_flush;
	}

protected:
    void _sink_it(const details::log_msg& msg) override
    {
        if (std::chrono::system_clock::now() >= _rotation_tp)
        {
            _file_helper.open(FileNameCalc::calc_filename(_base_filename));
            _rotation_tp = _next_rotation_tp();
        }
        _file_helper.write(msg);
		if(_force_flush)
			_file_helper.flush();
    }

    void _flush() override
    {
        _file_helper.flush();
    }

private:
    std::chrono::system_clock::time_point _next_rotation_tp()
    {
		auto now = std::chrono::system_clock::now();
		auto next = now + std::chrono::hours(1);
        time_t tnext = std::chrono::system_clock::to_time_t(next);
        tm date = spdlog::details::os::localtime(tnext);
        date.tm_min = 0;
        date.tm_sec = 0;
        auto rotation_time = std::chrono::system_clock::from_time_t(std::mktime(&date));
        return rotation_time;
    }

    filename_t _base_filename;
    int _rotation_h;
    int _rotation_m;
    std::chrono::system_clock::time_point _rotation_tp;
    details::file_helper _file_helper;
	bool _force_flush;
};

typedef hourly_file_sink<std::mutex> hourly_file_sink_mt;
typedef hourly_file_sink<details::null_mutex> hourly_file_sink_st;

} // end namespace extend
} // end namespace sinks
} // end namespace spdlog
