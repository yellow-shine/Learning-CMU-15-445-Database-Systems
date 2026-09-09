#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>
struct Row { int partition; int order; std::int64_t amount; };
enum class FrameUnit { Rows, Range, Groups };
enum class Bound { UnboundedPreceding, CurrentRow, UnboundedFollowing };
struct Frame { FrameUnit unit; Bound start; Bound end; };
struct WindowRow {
    Row row;
    std::size_t input_position;
    std::size_t row_number=0, rank=0, dense_rank=0;
    std::int64_t sum=0;
};
inline std::int64_t CheckedAdd(std::int64_t a, std::int64_t b) {
    if((b>0 && a>std::numeric_limits<std::int64_t>::max()-b) ||
       (b<0 && a<std::numeric_limits<std::int64_t>::min()-b))
        throw std::overflow_error("window SUM overflow");
    return a+b;
}
inline std::vector<WindowRow> Window(const std::vector<Row>& input, Frame frame) {
    if(frame.unit!=FrameUnit::Rows || frame.start!=Bound::UnboundedPreceding || frame.end!=Bound::CurrentRow)
        throw std::invalid_argument("only ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW supported");
    std::vector<WindowRow> output;
    for(std::size_t i=0;i<input.size();++i) output.push_back({input[i],i,0,0,0,0});
    std::stable_sort(output.begin(),output.end(),[](const WindowRow& a,const WindowRow& b) {
        return a.row.partition<b.row.partition ||
               (a.row.partition==b.row.partition && a.row.order<b.row.order);
    });
    std::size_t number=0, rank=0, dense=0;
    std::int64_t sum=0;
    for(std::size_t i=0;i<output.size();++i) {
        const bool first=i==0 || output[i].row.partition!=output[i-1].row.partition;
        if(first) { number=rank=dense=0; sum=0; }
        ++number;
        if(first || output[i].row.order!=output[i-1].row.order) { rank=number; ++dense; }
        sum=CheckedAdd(sum,output[i].row.amount);
        output[i].row_number=number; output[i].rank=rank; output[i].dense_rank=dense; output[i].sum=sum;
    }
    return output;
}
