#!/usr/bin/env python3
"""A small configurable cache simulator for NEMU memory traces.

The simulator supports direct mapped, N-way set associative, and fully
associative caches with FIFO or LRU replacement.  It parses log lines such as:

    load: addr = 0x100000000, len = 2, data = 0x93
    write: addr = 0x100003780, len = 4, data = 0x0

An access that spans multiple cache blocks is counted as one probe per block,
which matches the hardware event that more than one cache line may be touched.
"""

from __future__ import annotations

import argparse
import csv
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Iterator, Literal, TextIO


Policy = Literal["lru", "fifo"]

TRACE_RE = re.compile(
    r"\b(?P<op>load|write):\s+addr\s+=\s+0x(?P<addr>[0-9a-fA-F]+),\s+len\s+=\s+(?P<size>\d+)"
)


@dataclass
class CacheLine:
    valid: bool = False
    tag: int = -1
    inserted_at: int = 0
    last_used_at: int = 0


@dataclass
class CacheStats:
    accesses: int = 0
    hits: int = 0
    misses: int = 0
    reads: int = 0
    writes: int = 0

    @property
    def hit_rate(self) -> float:
        return self.hits / self.accesses if self.accesses else 0.0

    @property
    def miss_rate(self) -> float:
        return self.misses / self.accesses if self.accesses else 0.0

    def as_row(self) -> dict[str, int | float]:
        return {
            "accesses": self.accesses,
            "hits": self.hits,
            "misses": self.misses,
            "hit_rate": self.hit_rate,
            "miss_rate": self.miss_rate,
            "reads": self.reads,
            "writes": self.writes,
        }


class CacheSimulator:
    def __init__(
        self,
        cache_size: int,
        block_size: int,
        associativity: int | str,
        policy: Policy = "lru",
        write_allocate: bool = True,
    ) -> None:
        if block_size <= 0 or cache_size <= 0:
            raise ValueError("cache_size and block_size must be positive")
        if cache_size % block_size != 0:
            raise ValueError("cache_size must be a multiple of block_size")
        if policy not in ("lru", "fifo"):
            raise ValueError("policy must be either 'lru' or 'fifo'")

        self.cache_size = cache_size
        self.block_size = block_size
        self.line_count = cache_size // block_size
        self.policy: Policy = policy
        self.write_allocate = write_allocate

        if associativity == "full":
            self.associativity = self.line_count
        else:
            self.associativity = int(associativity)
        if self.associativity <= 0:
            raise ValueError("associativity must be positive")
        if self.line_count % self.associativity != 0:
            raise ValueError("line_count must be a multiple of associativity")

        self.set_count = self.line_count // self.associativity
        self.sets = [
            [CacheLine() for _ in range(self.associativity)] for _ in range(self.set_count)
        ]
        self.clock = 0
        self.stats = CacheStats()

    @property
    def mapping_name(self) -> str:
        if self.associativity == 1:
            return "direct"
        if self.associativity == self.line_count:
            return "fully_associative"
        return f"{self.associativity}_way"

    def _decode(self, addr: int) -> tuple[int, int]:
        block_addr = addr // self.block_size
        set_index = block_addr % self.set_count
        tag = block_addr // self.set_count
        return set_index, tag

    def _select_victim(self, lines: list[CacheLine]) -> CacheLine:
        for line in lines:
            if not line.valid:
                return line
        if self.policy == "fifo":
            return min(lines, key=lambda line: line.inserted_at)
        return min(lines, key=lambda line: line.last_used_at)

    def access_block(self, addr: int, is_write: bool = False) -> bool:
        self.clock += 1
        self.stats.accesses += 1
        if is_write:
            self.stats.writes += 1
        else:
            self.stats.reads += 1

        set_index, tag = self._decode(addr)
        lines = self.sets[set_index]
        for line in lines:
            if line.valid and line.tag == tag:
                self.stats.hits += 1
                line.last_used_at = self.clock
                return True

        self.stats.misses += 1
        if (not is_write) or self.write_allocate:
            victim = self._select_victim(lines)
            victim.valid = True
            victim.tag = tag
            victim.inserted_at = self.clock
            victim.last_used_at = self.clock
        return False

    def access(self, addr: int, size: int = 1, is_write: bool = False) -> list[bool]:
        if size <= 0:
            return []
        start_block = addr // self.block_size
        end_block = (addr + size - 1) // self.block_size
        return [
            self.access_block(block * self.block_size, is_write)
            for block in range(start_block, end_block + 1)
        ]

    def run(self, accesses: Iterable[tuple[str, int, int]]) -> CacheStats:
        for op, addr, size in accesses:
            self.access(addr, size, is_write=(op == "write"))
        return self.stats


def parse_size(value: str) -> int:
    text = value.strip().lower()
    multipliers = {
        "b": 1,
        "k": 1024,
        "kb": 1024,
        "m": 1024 * 1024,
        "mb": 1024 * 1024,
    }
    for suffix in ("kb", "mb", "k", "m", "b"):
        if text.endswith(suffix):
            return int(text[: -len(suffix)]) * multipliers[suffix]
    return int(text)


def parse_trace(handle: TextIO) -> Iterator[tuple[str, int, int]]:
    for line in handle:
        match = TRACE_RE.search(line)
        if match:
            op = "write" if match.group("op") == "write" else "load"
            yield op, int(match.group("addr"), 16), int(match.group("size"))


def iter_trace_file(path: Path) -> Iterator[tuple[str, int, int]]:
    with path.open("r", encoding="utf-8", errors="ignore") as handle:
        yield from parse_trace(handle)


def simulate_file(
    trace_path: Path,
    cache_size: int,
    block_size: int,
    associativity: int | str,
    policy: Policy,
    write_allocate: bool = True,
) -> tuple[CacheSimulator, CacheStats]:
    sim = CacheSimulator(
        cache_size=cache_size,
        block_size=block_size,
        associativity=associativity,
        policy=policy,
        write_allocate=write_allocate,
    )
    return sim, sim.run(iter_trace_file(trace_path))


def write_csv(path: Path, rows: list[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fieldnames = list(rows[0].keys()) if rows else []
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def self_test() -> None:
    # Direct mapped cache: blocks 0 and 2 conflict in set 0, block 1 stays in set 1.
    direct = CacheSimulator(cache_size=8, block_size=4, associativity=1, policy="lru")
    direct.run(("load", addr, 1) for addr in [0, 4, 8, 0, 4, 8])
    assert (direct.stats.hits, direct.stats.misses) == (1, 5), direct.stats

    # Same fully associative cache, different victim choice for FIFO and LRU.
    seq = [0, 4, 0, 8, 4, 0]
    lru = CacheSimulator(cache_size=8, block_size=4, associativity="full", policy="lru")
    fifo = CacheSimulator(cache_size=8, block_size=4, associativity="full", policy="fifo")
    lru.run(("load", addr, 1) for addr in seq)
    fifo.run(("load", addr, 1) for addr in seq)
    assert (lru.stats.hits, lru.stats.misses) == (1, 5), lru.stats
    assert (fifo.stats.hits, fifo.stats.misses) == (2, 4), fifo.stats

    # A multi-byte access crossing a block boundary should touch two cache lines.
    cross = CacheSimulator(cache_size=16, block_size=4, associativity=1)
    cross.run([("load", 2, 4), ("load", 3, 1)])
    assert (cross.stats.accesses, cross.stats.hits, cross.stats.misses) == (3, 1, 2), cross.stats


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Cache simulator for NEMU trace.log files")
    parser.add_argument("trace", nargs="?", type=Path, help="trace.log path")
    parser.add_argument("--cache-size", default="4KB", help="total cache size, e.g. 4KB")
    parser.add_argument("--block-size", default="32B", help="cache block size, e.g. 32B")
    parser.add_argument(
        "--assoc",
        default="1",
        help="'1' for direct mapped, N for N-way set associative, or 'full'",
    )
    parser.add_argument("--policy", choices=["lru", "fifo"], default="lru")
    parser.add_argument(
        "--no-write-allocate",
        action="store_true",
        help="do not allocate a line on write misses",
    )
    parser.add_argument("--self-test", action="store_true", help="run micro-benchmark checks")
    return parser


def main() -> None:
    parser = build_arg_parser()
    args = parser.parse_args()
    if args.self_test:
        self_test()
        print("self-test passed")
        if args.trace is None:
            return
    if args.trace is None:
        parser.error("trace path is required unless --self-test is used alone")

    assoc: int | str = "full" if args.assoc.lower() == "full" else int(args.assoc)
    sim, stats = simulate_file(
        trace_path=args.trace,
        cache_size=parse_size(args.cache_size),
        block_size=parse_size(args.block_size),
        associativity=assoc,
        policy=args.policy,
        write_allocate=not args.no_write_allocate,
    )
    print(
        f"mapping={sim.mapping_name}, policy={args.policy}, "
        f"cache_size={sim.cache_size}, block_size={sim.block_size}, "
        f"accesses={stats.accesses}, hits={stats.hits}, misses={stats.misses}, "
        f"hit_rate={stats.hit_rate:.6f}, miss_rate={stats.miss_rate:.6f}"
    )


if __name__ == "__main__":
    main()
